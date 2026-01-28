# Лабораторная работа 5: Web-интерфейс

Данный проект является продолжением 4-й лабораторной работы. 
## Сборка проекта

### Windows (Powershell)
```bash
cmake -B build -S . -G "MinGW Makefiles" ; cmake --build build
```

### Linux
```bash
cmake -B build -S . && cmake --build build
```

## Запуск

Для автоматизации запуска в директории scripts находятся скрипты.

- **Windows:** `.\scripts\run_windows.bat`
- **Linux:** `./scripts/run_posix.sh`