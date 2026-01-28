@echo off
echo ==============================================
echo   Starting Weather Station
echo ==============================================

cd build

call :KILL_PROCESSES

set QT_PATH=C:\msys64\mingw64
set PATH=%QT_PATH%\bin;%PATH%
set QT_QPA_PLATFORM_PLUGIN_PATH=.\platforms

echo Starting sensor simulator...
start "Sensor Simulator" /B sensor_simulator.exe
timeout /t 1 /nobreak >nul

echo Starting weather server...
start "Weather Server" /B weather_server.exe virtual_com
timeout /t 2 /nobreak >nul

echo Starting GUI client...
echo.
echo ==============================================
echo Weather Station is running!
echo Close the GUI window to stop all processes.
echo ==============================================
echo.
weather_gui.exe

echo GUI closed. Stopping all processes...
call :KILL_PROCESSES

echo All processes stopped.
cd ..
pause
exit /b 0

:KILL_PROCESSES
echo Stopping processes...
powershell -Command "Stop-Process -Name 'weather_gui' -Force -ErrorAction SilentlyContinue"
powershell -Command "Stop-Process -Name 'weather_server' -Force -ErrorAction SilentlyContinue"
powershell -Command "Stop-Process -Name 'sensor_simulator' -Force -ErrorAction SilentlyContinue"
taskkill //F //IM weather_gui.exe 2>nul
taskkill //F //IM weather_server.exe 2>nul
taskkill //F //IM sensor_simulator.exe 2>nul
exit /b