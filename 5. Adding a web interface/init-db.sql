-- Создание таблиц для хранения данных о температуре
CREATE TABLE IF NOT EXISTS measurements (
                                            id SERIAL PRIMARY KEY,
                                            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                                            temperature REAL NOT NULL
);

CREATE TABLE IF NOT EXISTS hourly_averages (
                                               id SERIAL PRIMARY KEY,
                                               timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                                               average_temperature REAL NOT NULL
);

CREATE TABLE IF NOT EXISTS daily_averages (
                                              id SERIAL PRIMARY KEY,
                                              timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                                              average_temperature REAL NOT NULL
);

-- Индексы для ускорения запросов
CREATE INDEX IF NOT EXISTS idx_measurements_timestamp ON measurements(timestamp);
CREATE INDEX IF NOT EXISTS idx_hourly_timestamp ON hourly_averages(timestamp);
CREATE INDEX IF NOT EXISTS idx_daily_timestamp ON daily_averages(timestamp);

-- Функции для очистки старых данных
CREATE OR REPLACE FUNCTION cleanup_old_measurements()
RETURNS void AS $$
BEGIN
DELETE FROM measurements WHERE timestamp < NOW() - INTERVAL '30 days';
DELETE FROM hourly_averages WHERE timestamp < NOW() - INTERVAL '90 days';
END;
$$ LANGUAGE plpgsql;

-- Представление для последних измерений
CREATE OR REPLACE VIEW recent_measurements AS
SELECT timestamp, temperature
FROM measurements
ORDER BY timestamp DESC
    LIMIT 1000;

-- Комментарии
COMMENT ON TABLE measurements IS 'Сырые измерения температуры';
COMMENT ON TABLE hourly_averages IS 'Средние значения температуры за каждый час';
COMMENT ON TABLE daily_averages IS 'Средние значения температуры за каждый день';