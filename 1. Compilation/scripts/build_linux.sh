#!/bin/bash

echo "Обновление кода из Git..."
git pull 2>/dev/null || echo "Git update skipped"

echo "Очистка предыдущей сборки..."
rm -rf build

echo "Создание папки build..."
mkdir -p build
cd build

echo "Сборка проекта..."
cmake .. >/dev/null 2>&1
make -j$(nproc)

echo ""
echo "Сборка завершена!"
echo Запуск программы...
echo ""
./bin/OS_Detector