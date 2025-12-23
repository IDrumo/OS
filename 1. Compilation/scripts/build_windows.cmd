@echo off
chcp 65001 >nul
echo.

echo Обновление репозитория...
git pull 2>nul || echo Git update skipped

echo Очистка предыдущей сборки...
if exist "build" (
    rmdir /s /q build
    echo Папка 'build' удалена.
)

echo Конфигурация CMake...
cmake -S . -B build -G "MinGW Makefiles"
echo Сборка проекта...
cmake --build build --parallel

echo.
echo Сборка завершена!
echo Запуск программы...
echo.
"build\bin\OS_Detector.exe"