#!/bin/bash

echo "Creating virtual COM port..."
mkfifo virtual_com 2>/dev/null || true

echo "Starting sensor simulator..."
./build/sensor_simulator > virtual_com &
SIM_PID=$!

sleep 2

echo "Starting weather server..."
./build/weather_server virtual_com &
SERVER_PID=$!

sleep 1

echo ""
echo "========================================"
echo "Weather Station is running!"
echo "Web Interface: http://localhost:8080"
echo ""
echo "Process PIDs:"
echo "  Simulator: $SIM_PID"
echo "  Server: $SERVER_PID"
echo "========================================"
echo ""
echo "Press Ctrl+C to stop all services..."

# Обработчик прерывания
cleanup() {
    echo ""
    echo "Stopping Weather Station services..."
    kill $SERVER_PID 2>/dev/null && echo "Server stopped"
    kill $SIM_PID 2>/dev/null && echo "Simulator stopped"
    rm -f virtual_com && echo "Virtual port removed"
    echo "All services stopped."
    exit 0
}

trap cleanup SIGINT SIGTERM

# Бесконечное ожидание
while true; do
    sleep 1
done