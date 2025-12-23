@echo off
chcp 65001 >nul
echo.

set TARGET_DIR=%~1
if "%TARGET_DIR%"=="" set TARGET_DIR=.
set EXEC_NAME=%~2
if "%EXEC_NAME%"=="" set EXEC_NAME=app

cd /d "%TARGET_DIR%" 2>nul || exit /b 1

echo ==============================
echo Сборка проекта в: %CD%
echo Имя исполняемого файла: %EXEC_NAME%
echo ==============================

rmdir /s /q build 2>nul
git fetch 2>nul & git pull --ff-only 2>nul || echo Using local
cmake -B build -S . -G "MinGW Makefiles" && cmake --build build && build\%EXEC_NAME%.exe
