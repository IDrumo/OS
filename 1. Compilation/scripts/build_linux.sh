#!/bin/bash

echo "Обновление кода из Git..."
git pull 2>/dev/null || echo "Git update skipped"

echo "Создание папки build..."
mkdir -p build
cd build

echo "Сборка проекта..."
cmake .. >/dev/null 2>&1
make -j$(nproc)

echo ""
echo "Сборка завершена!"
./bin/OS_Detector