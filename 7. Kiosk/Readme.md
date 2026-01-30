# Лабораторная работа 6: Графический клиент (GUI)

Данный проект является продолжением 5-й лабораторной работы. К существующему HTTP-серверу с базой данных SQLite добавлено кроссплатформенное десктопное приложение с графическим интерфейсом (GUI) на C++ с использованием библиотеки **Qt (Qt Charts)**.

### Установка зависимостей
- **Windows (MSYS2 MinGW64):**
  ```bash
  pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-toolchain mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-charts
  ```
- **Linux (Ubuntu 20.04+):**
  ```bash
  sudo apt update
  sudo apt install build-essential cmake qtbase5-dev libqt5charts5-dev
  ```

## Сборка проекта
Для сборки, запуска и остановки приложения в проекте предусмотрены соответствующие скрипты в директории `scripts`. 
Запускать скрипты рекомендуется из корня проекта

### Windows (Powershell)
```bash
.\scripts\build_windows.bat 
```
```bash
.\scripts\run_windows.bat
```
```bash
.\scripts\stop_windows.bat
```

### Linux
```bash
./scripts/build_posix.sh
```
```bash
./scripts/run_posix.sh
```
```bash
./scripts/stop_windows.sh
```
