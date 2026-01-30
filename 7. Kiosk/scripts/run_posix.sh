#!/bin/bash
echo "=============================================="
echo "   Starting Weather Station"
echo "=============================================="

cd build

pkill -f weather_gui 2>/dev/null
pkill -f weather_server 2>/dev/null
pkill -f sensor_simulator 2>/dev/null

echo "Starting sensor simulator..."
./sensor_simulator &
SIM_PID=$!

sleep 1

echo "Starting weather server..."
./weather_server virtual_com &
SERVER_PID=$!

sleep 2

echo "Starting GUI client..."
echo ""
echo "=============================================="
echo "Weather Station is running!"
echo "Close the GUI window to stop all processes."
echo "=============================================="
echo ""
./weather_gui

echo "GUI closed. Stopping all processes..."
kill $SIM_PID 2>/dev/null
kill $SERVER_PID 2>/dev/null

pkill -f weather_gui 2>/dev/null
pkill -f weather_server 2>/dev/null
pkill -f sensor_simulator 2>/dev/null

echo "All processes stopped."
cd ..