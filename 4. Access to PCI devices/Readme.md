# Temperature Logger

Кроссплатформенная программа для логирования температурных данных с устройства или симулятора.

## Сборка

```bash
mkdir build && cd build
cmake .. && make
```

## Запуск

### С симулятором устройства
**CMD (Windows):**
```cmd
device_simulator.exe | temp_logger.exe
```

**PowerShell:**
```powershell
cmd /c "device_simulator.exe | temp_logger.exe"
```

**Bash (Linux/macOS):**
```bash
./device_simulator | ./temp_logger
```

### С реальным устройством
```bash
# Linux/macOS
./temp_logger --port /dev/ttyUSB0

# Windows
temp_logger.exe --port COM3
```

## Просмотр логов

Логи сохраняются в папке `logs/`:
- `measurements.log` — все измерения за последние 24 часа
- `hourly.log` — средние за час за последние 30 дней
- `daily.log` — средние за день за текущий год

```bash
# Посмотреть последние измерения
tail -f logs/measurements.log

# Показать все логи
ls logs/
```