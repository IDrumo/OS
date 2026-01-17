# Temperature Monitoring System v2.0

## Описание
Кроссплатформенная система мониторинга температуры с веб-интерфейсом и базой данных PostgreSQL.

## Архитектура
- **C++ приложение**: Чтение данных с COM-порта, обработка, сохранение в БД, REST API
- **PostgreSQL**: Хранение всех измерений и статистики
- **Python/Flask веб-приложение**: Веб-интерфейс с графиками и таблицами
- **Docker Compose**: Развертывание PostgreSQL и веб-приложения

## Быстрый старт

### 1. Установка зависимостей
```bash
# Linux (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install docker docker-compose libpqxx-dev cmake g++

# Windows
# Установите Docker Desktop, MinGW, CMake
```

### 2. Клонирование и настройка
```bash
git clone <repository>
cd temperature-monitor
cp .env.example .env
# Отредактируйте .env при необходимости
```

### 3. Запуск системы
```bash
# Linux/Mac
./run.sh

# Windows
run.bat
```

### 4. Запуск C++ приложения
```bash
# В отдельном терминале
cd build
./temperature_monitor --simulate  # Режим симуляции
# Или с реальным портом
./temperature_monitor COM1 --baud 115200
```

## API Endpoints

### C++ HTTP Server (порт 8080)
- `GET /api/current` - текущая температура
- `GET /api/measurements?limit=100` - последние измерения
- `GET /api/hourly?days=7` - часовые средние
- `GET /api/daily?days=30` - дневные средние
- `GET /api/statistics?period=day` - статистика за период
- `GET /api/health` - состояние системы

### Python Web API (порт 5000)
- Все эндпоинты C++ API плюс:
- `GET /api/alerts?min=10&max=30` - предупреждения
- `GET /api/system_info` - информация о системе

## Веб-интерфейс
Откройте в браузере: http://localhost:5000

## Структура проекта
```
cpp/              - Исходный код C++ приложения
web/              - Python веб-приложение
docker-compose.yml - Конфигурация Docker
init-db.sql      - SQL для инициализации БД
.env.example     - Пример переменных окружения
```

## Разработка

### Сборка C++ приложения
```bash
cd cpp
mkdir build && cd build
cmake ..
make
```

### Тестирование
```bash
# Запуск симулятора
./simulator COM1 1000

# Запуск тестовой программы
./test_monitor
```
