#!/bin/bash

cd build

echo "Creating virtual COM port..."
rm -f virtual_com
mkfifo virtual_com

echo "Starting sensor simulator..."
./sensor_simulator &
SIM_PID=$!

sleep 2

echo "Starting weather server..."
./weather_server virtual_com &
SERVER_PID=$!

cd ..

sleep 3

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
echo "Press ENTER to stop all services..."

read

echo ""
echo "Stopping Weather Station services..."

kill $SERVER_PID 2>/dev/null
kill $SIM_PID 2>/dev/null

rm -f build/virtual_com 2>/dev/null

echo "All services stopped."