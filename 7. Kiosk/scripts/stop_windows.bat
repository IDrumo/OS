@echo off
echo ==============================================
echo   Stopping Weather Station
echo ==============================================

echo Using PowerShell to forcefully stop all processes...

powershell -Command "Write-Host 'Stopping weather_gui...' -ForegroundColor Yellow; Stop-Process -Name 'weather_gui' -Force -ErrorAction SilentlyContinue; Write-Host 'Stopping weather_server...' -ForegroundColor Yellow; Stop-Process -Name 'weather_server' -Force -ErrorAction SilentlyContinue; Write-Host 'Stopping sensor_simulator...' -ForegroundColor Yellow; Stop-Process -Name 'sensor_simulator' -Force -ErrorAction SilentlyContinue; Write-Host 'All processes stopped.' -ForegroundColor Green"

echo.
echo Double-checking with taskkill...
taskkill //F //IM weather_gui.exe 2>nul && echo weather_gui.exe stopped
taskkill //F //IM weather_server.exe 2>nul && echo weather_server.exe stopped
taskkill //F //IM sensor_simulator.exe 2>nul && echo sensor_simulator.exe stopped

echo.
echo ==============================================
echo Weather Station processes have been stopped.
echo ==============================================
pause