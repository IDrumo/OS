#!/bin/bash
echo "=============================================="
echo "   Stopping Weather Station"
echo "=============================================="

echo "Stopping processes..."

pkill -f weather_gui 2>/dev/null
pkill -f weather_server 2>/dev/null
pkill -f sensor_simulator 2>/dev/null

sleep 2
pkill -9 -f weather_gui 2>/dev/null
pkill -9 -f weather_server 2>/dev/null
pkill -9 -f sensor_simulator 2>/dev/null

echo "Checking for running processes..."

PROCESSES=("weather_gui" "weather_server" "sensor_simulator")
ALL_STOPPED=true

for proc in "${PROCESSES[@]}"; do
    if pgrep -f "$proc" > /dev/null; then
        echo "WARNING: $proc is still running!"
        ALL_STOPPED=false
    fi
done

if [ "$ALL_STOPPED" = true ]; then
    echo "All Weather Station processes have been stopped."
else
    echo "Some processes may still be running. Try:"
    echo "1. Run this script with sudo"
    echo "2. Manually check with: ps aux | grep weather"
    echo "3. Reboot if necessary"
fi

echo "=============================================="