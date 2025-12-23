#!/bin/bash

TARGET_DIR="${1:-.}"
EXEC_NAME="${2:-app}"

cd "$TARGET_DIR" || exit 1

echo "=============================="
echo "Сборка проекта в: $(pwd)"
echo "Имя исполняемого файла: $EXEC_NAME"
echo "=============================="

rm -rf build 2>/dev/null
git fetch 2>/dev/null; git pull --ff-only 2>/dev/null || true
cmake -B build -S . && cmake --build build && ./build/"$EXEC_NAME"