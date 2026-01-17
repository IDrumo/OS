#!/bin/bash

echo "Starting Temperature Monitoring System with Docker Compose..."

# Копирование .env файла если его нет
if [ ! -f .env ]; then
    echo "Creating .env file from example..."
    cp .env.example .env
    echo "Please edit .env file with your configuration"
fi

# Запуск PostgreSQL и веб-приложения в Docker
echo "Starting Docker containers..."
docker-compose up -d

# Ожидание запуска PostgreSQL
echo "Waiting for PostgreSQL to start..."
sleep 5

# Сборка C++ приложения
echo "Building C++ application..."
mkdir -p build
cd build
cmake ../cpp
make -j$(nproc)
cd ..

# Проверка существования базы данных
echo "Testing database connection..."
if ! docker-compose exec postgres psql -U temperature -d temperature -c "\dt" > /dev/null 2>&1; then
    echo "Database not initialized. Running init script..."
    docker-compose exec postgres psql -U temperature -d temperature -f /docker-entrypoint-initdb.d/init.sql
fi

# Запуск C++ программы
echo "Starting C++ temperature monitor..."
echo "Note: The monitor will use connection string from .env file"
echo "To run with simulation mode: ./build/temperature_monitor --simulate"
echo "To run with real port: ./build/temperature_monitor COM1"
echo ""
echo "Services:"
echo "- PostgreSQL: localhost:5432"
echo "- Web Interface: http://localhost:5000"
echo "- C++ HTTP API: http://localhost:8080"
echo ""
echo "Press Ctrl+C to stop all services"

# Бесконечное ожидание
trap "echo 'Stopping services...'; docker-compose down" EXIT
sleep infinity