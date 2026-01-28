@echo off
setlocal enabledelayedexpansion

cd build
echo Creating virtual COM port...
echo. > virtual_com

echo Starting sensor simulator...
start "Sensor Simulator" /B sensor_simulator.exe
timeout /t 2 /nobreak >nul

echo Starting weather server...
start "Weather Server" /B weather_server.exe virtual_com
cd ..
timeout /t 3 /nobreak >nul

for /f "tokens=2" %%i in ('tasklist ^| findstr /i "sensor_simulator"') do set SIM_PID=%%i
for /f "tokens=2" %%i in ('tasklist ^| findstr /i "weather_server"') do set SERVER_PID=%%i

echo.
echo ========================================
echo Weather Station is running!
echo Web Interface: http://localhost:8080
echo.
echo Process PIDs:
echo   Simulator: %SIM_PID%
echo   Server: %SERVER_PID%
echo ========================================
echo.
echo Press ANY KEY to stop all services...

pause >nul

echo.
echo Stopping Weather Station services...

if defined SERVER_PID taskkill /PID %SERVER_PID% /F >nul 2>&1
if defined SIM_PID taskkill /PID %SIM_PID% /F >nul 2>&1

taskkill /F /IM weather_server.exe >nul 2>&1
taskkill /F /IM sensor_simulator.exe >nul 2>&1

del virtual_com >nul 2>&1
cd scripts

echo All services stopped.
echo.
pause
endlocal