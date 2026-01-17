#!/usr/bin/env python3
"""
Temperature Monitor Web Interface with PostgreSQL
"""
from flask import Flask, render_template, jsonify, request
import psycopg2
from psycopg2.extras import RealDictCursor
from datetime import datetime, timedelta
import json
import os
from dotenv import load_dotenv
import logging

# Настройка логирования
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Загрузка переменных окружения
load_dotenv()

app = Flask(__name__)

# Конфигурация базы данных из переменных окружения
def get_db_config():
    return {
        'dbname': os.getenv('POSTGRES_DB', 'temperature'),
        'user': os.getenv('POSTGRES_USER', 'temperature'),
        'password': os.getenv('POSTGRES_PASSWORD', 'temperature'),
        'host': os.getenv('POSTGRES_HOST', 'localhost'),
        'port': os.getenv('POSTGRES_PORT', '5432')
    }

def get_db_connection():
    """Создание соединения с базой данных"""
    try:
        conn = psycopg2.connect(**get_db_config())
        return conn
    except Exception as e:
        logger.error(f"Database connection error: {e}")
        return None

@app.route('/')
def index():
    """Главная страница с графиками"""
    return render_template('index.html')

@app.route('/api/current')
def get_current():
    """Текущая температура"""
    conn = get_db_connection()
    if not conn:
        return jsonify({'error': 'Database connection failed'}), 500

    try:
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        cursor.execute('''
                       SELECT timestamp, temperature
                       FROM measurements
                       ORDER BY timestamp DESC
                           LIMIT 1
                       ''')

        row = cursor.fetchone()
        cursor.close()
        conn.close()

        if row:
            return jsonify({
                'success': True,
                'data': {
                    'timestamp': row['timestamp'].isoformat(),
                    'temperature': float(row['temperature'])
                }
            })
        return jsonify({'success': False, 'error': 'No data available'})
    except Exception as e:
        logger.error(f"Error getting current temperature: {e}")
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/measurements')
def get_measurements():
    """Последние измерения"""
    limit = request.args.get('limit', default=100, type=int)

    conn = get_db_connection()
    if not conn:
        return jsonify({'error': 'Database connection failed'}), 500

    try:
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        cursor.execute('''
                       SELECT timestamp, temperature
                       FROM measurements
                       ORDER BY timestamp DESC
                           LIMIT %s
                       ''', (limit,))

        rows = cursor.fetchall()
        cursor.close()
        conn.close()

        data = [
            {
                'timestamp': row['timestamp'].isoformat(),
                'temperature': float(row['temperature'])
            }
            for row in rows
        ]

        return jsonify({
            'success': True,
            'count': len(data),
            'data': data
        })
    except Exception as e:
        logger.error(f"Error getting measurements: {e}")
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/hourly')
def get_hourly():
    """Средние по часам"""
    days = request.args.get('days', default=7, type=int)

    conn = get_db_connection()
    if not conn:
        return jsonify({'error': 'Database connection failed'}), 500

    try:
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        cursor.execute('''
                       SELECT timestamp, average_temperature as temperature
                       FROM hourly_averages
                       WHERE timestamp >= NOW() - INTERVAL %s
                       ORDER BY timestamp
                       ''', (f'{days} days',))

        rows = cursor.fetchall()
        cursor.close()
        conn.close()

        data = [
            {
                'timestamp': row['timestamp'].isoformat(),
                'temperature': float(row['temperature'])
            }
            for row in rows
        ]

        return jsonify({
            'success': True,
            'count': len(data),
            'data': data
        })
    except Exception as e:
        logger.error(f"Error getting hourly averages: {e}")
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/daily')
def get_daily():
    """Средние по дням"""
    days = request.args.get('days', default=30, type=int)

    conn = get_db_connection()
    if not conn:
        return jsonify({'error': 'Database connection failed'}), 500

    try:
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        cursor.execute('''
                       SELECT DATE(timestamp) as date,
                           AVG(average_temperature) as temperature
                       FROM daily_averages
                       WHERE timestamp >= NOW() - INTERVAL %s
                       GROUP BY DATE(timestamp)
                       ORDER BY date
                       ''', (f'{days} days',))

        rows = cursor.fetchall()
        cursor.close()
        conn.close()

        data = [
            {
                'timestamp': row['date'].isoformat(),
                'temperature': float(row['temperature'])
            }
            for row in rows
        ]

        return jsonify({
            'success': True,
            'count': len(data),
            'data': data
        })
    except Exception as e:
        logger.error(f"Error getting daily averages: {e}")
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/statistics')
def get_statistics():
    """Статистика за различные периоды"""
    period = request.args.get('period', default='day')

    conn = get_db_connection()
    if not conn:
        return jsonify({'error': 'Database connection failed'}), 500

    try:
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        if period == 'hour':
            cursor.execute('''
                           SELECT AVG(temperature) as avg_temp,
                                  MIN(temperature) as min_temp,
                                  MAX(temperature) as max_temp
                           FROM measurements
                           WHERE timestamp >= NOW() - INTERVAL '1 hour'
                           ''')
        elif period == 'day':
            cursor.execute('''
                           SELECT AVG(temperature) as avg_temp,
                                  MIN(temperature) as min_temp,
                                  MAX(temperature) as max_temp
                           FROM measurements
                           WHERE timestamp >= NOW() - INTERVAL '1 day'
                           ''')
        elif period == 'week':
            cursor.execute('''
                           SELECT AVG(average_temperature) as avg_temp,
                                  MIN(average_temperature) as min_temp,
                                  MAX(average_temperature) as max_temp
                           FROM hourly_averages
                           WHERE timestamp >= NOW() - INTERVAL '7 days'
                           ''')
        elif period == 'month':
            cursor.execute('''
                           SELECT AVG(average_temperature) as avg_temp,
                                  MIN(average_temperature) as min_temp,
                                  MAX(average_temperature) as max_temp
                           FROM daily_averages
                           WHERE timestamp >= NOW() - INTERVAL '30 days'
                           ''')
        else:
            return jsonify({'success': False, 'error': f'Invalid period: {period}'}), 400

        row = cursor.fetchone()
        cursor.close()
        conn.close()

        if row and row['avg_temp'] is not None:
            return jsonify({
                'success': True,
                'data': {
                    'period': period,
                    'average': float(row['avg_temp']),
                    'minimum': float(row['min_temp']) if row['min_temp'] else None,
                    'maximum': float(row['max_temp']) if row['max_temp'] else None
                }
            })

        return jsonify({'success': False, 'error': f'No data available for period: {period}'})
    except Exception as e:
        logger.error(f"Error getting statistics: {e}")
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/alerts')
def get_alerts():
    """Проверка на выход за допустимые пределы"""
    threshold_min = request.args.get('min', default=10, type=float)
    threshold_max = request.args.get('max', default=30, type=float)
    hours = request.args.get('hours', default=24, type=int)

    conn = get_db_connection()
    if not conn:
        return jsonify({'error': 'Database connection failed'}), 500

    try:
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        cursor.execute('''
                       SELECT timestamp, temperature
                       FROM measurements
                       WHERE timestamp >= NOW() - INTERVAL %s
                         AND (temperature < %s OR temperature > %s)
                       ORDER BY timestamp DESC
                       ''', (f'{hours} hours', threshold_min, threshold_max))

        rows = cursor.fetchall()
        cursor.close()
        conn.close()

        alerts = [
            {
                'timestamp': row['timestamp'].isoformat(),
                'temperature': float(row['temperature']),
                'type': 'too_low' if row['temperature'] < threshold_min else 'too_high'
            }
            for row in rows
        ]

        return jsonify({
            'success': True,
            'data': {
                'threshold_min': threshold_min,
                'threshold_max': threshold_max,
                'hours': hours,
                'alerts': alerts,
                'count': len(alerts)
            }
        })
    except Exception as e:
        logger.error(f"Error getting alerts: {e}")
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/system_info')
def get_system_info():
    """Информация о системе"""
    conn = get_db_connection()
    if not conn:
        return jsonify({'error': 'Database connection failed'}), 500

    try:
        cursor = conn.cursor(cursor_factory=RealDictCursor)

        # Получаем статистику по таблицам
        cursor.execute('''
                       SELECT 'measurements' as table_name, COUNT(*) as count, 
                   MIN(timestamp) as min_date, MAX(timestamp) as max_date
                       FROM measurements
                       UNION ALL
                       SELECT 'hourly_averages' as table_name, COUNT(*) as count,
                   MIN(timestamp) as min_date, MAX(timestamp) as max_date
                       FROM hourly_averages
                       UNION ALL
                       SELECT 'daily_averages' as table_name, COUNT(*) as count,
                   MIN(timestamp) as min_date, MAX(timestamp) as max_date
                       FROM daily_averages
                       ''')

        rows = cursor.fetchall()

        # Получаем информацию о БД
        cursor.execute('SELECT version() as version')
        db_version = cursor.fetchone()['version']

        cursor.close()
        conn.close()

        return jsonify({
            'success': True,
            'data': {
                'database': {
                    'version': db_version,
                    'host': get_db_config()['host'],
                    'name': get_db_config()['dbname']
                },
                'tables': [
                    {
                        'name': row['table_name'],
                        'count': row['count'],
                        'min_date': row['min_date'].isoformat() if row['min_date'] else None,
                        'max_date': row['max_date'].isoformat() if row['max_date'] else None
                    }
                    for row in rows
                ],
                'timestamp': datetime.now().isoformat()
            }
        })
    except Exception as e:
        logger.error(f"Error getting system info: {e}")
        return jsonify({'success': False, 'error': str(e)}), 500

if __name__ == '__main__':
    logger.info("Starting Temperature Monitor Web Interface...")
    logger.info(f"Database config: {get_db_config()['host']}:{get_db_config()['port']}/{get_db_config()['dbname']}")
    logger.info("Open http://localhost:5000 in your browser")

    app.run(debug=os.getenv('FLASK_DEBUG', 'False').lower() == 'true',
            host='0.0.0.0',
            port=5000)