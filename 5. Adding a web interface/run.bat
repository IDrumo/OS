@echo off
echo Starting Temperature Monitoring System with Docker Compose...

REM Копирование .env файла если его нет
if not exist .env (
    echo Creating .env file from example...
    copy .env.example .env
    echo Please edit .env file with your configuration
)

REM Запуск Docker Compose
echo Starting Docker containers...
docker-compose up -d

REM Ожидание запуска PostgreSQL
echo Waiting for PostgreSQL to start...
timeout /t 5

REM Сборка C++ приложения
echo Building C++ application...
mkdir build 2>nul
cd build
cmake ../cpp -G "MinGW Makefiles"
cmake --build .
cd ..

echo.
echo Services:
echo - PostgreSQL: localhost:5432
echo - Web Interface: http://localhost:5000
echo - C++ HTTP API: http://localhost:8080
echo.
echo To start C++ monitor with simulation:
echo   build\temperature_monitor.exe --simulate
echo.
echo To start C++ monitor with real port:
echo   build\temperature_monitor.exe COM1
echo.
pause

REM Остановка контейнеров
echo Stopping Docker containers...
docker-compose down