# Temperature Monitoring System

## Описание
Кроссплатформенная программа для мониторинга температуры с COM-порта.

## Возможности
- Чтение температуры с серийного порта/USB
- Валидация данных (диапазон, контрольная сумма)
- Логирование в три файла:
  - `measurements.log` - все измерения за 24 часа
  - `hourly.log` - средние за час за 30 дней
  - `daily.log` - средние за день за год
- Ротация лог-файлов
- Режим симуляции для тестирования

## Сборка
```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"  # для Windows
cmake ..                        # для Linux
cmake --build .
```

## Использование
### Основная программа:
```bash
# С реальным устройством
./temperature_monitor COM1

# В режиме симуляции
./temperature_monitor --simulate

# С указанием скорости порта
./temperature_monitor COM1 --baud 9600
```

### Симулятор устройства:
```bash
./simulator COM1 1000
```

## Тестирование
```bash
./test_monitor.exe      # Unit-тесты
```