@echo off
chcp 65001 >nul
echo.

set REPO_DIR=%CD%

echo Очистка предыдущей сборки...
if exist "build" (
    rmdir /s /q build
    echo Папка 'build' удалена.
)

echo [1/3] Обновление репозитория...
git pull 2>nul || echo Не удалось обновить или не Git репозиторий
echo [2/3] Конфигурация CMake...
cmake -S . -B build -G "MinGW Makefiles"
echo [3/3] Сборка проекта...
cmake --build build --parallel

echo.
echo Сборка завершена!
echo Запуск программы...
echo.
if exist "%REPO_DIR%\build\bin\OS_Detector.exe" (
  "%REPO_DIR%\build\bin\OS_Detector.exe"
) else if exist "%REPO_DIR%\build\OS_Detector.exe" (
  "%REPO_DIR%\build\OS_Detector.exe"
) else (
  echo Исполняемый файл не найден.
  exit /b 1
)