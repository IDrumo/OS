#!/bin/bash

if [ "$EUID" -ne 0 ]; then
    echo "Запустите с sudo: sudo ./install_deps.sh"
    exit 1
fi

echo "==============================================="
echo " Установка минимальных зависимостей"
echo "==============================================="

apt update -y
apt upgrade -y

echo "Установка build-essential, cmake, git..."
apt install -y \
    build-essential \
    cmake \
    git

echo "Установка Qt5 компонентов..."
apt install -y \
    qtbase5-dev \
    qt5-default \
    qttools5-dev-tools \
    libqt5charts5-dev \
    qtcharts5-dev-tools \
    libqt5network5 \
    libqt5core5a

echo "Установка SQLite..."
apt install -y \
    sqlite3 \
    libsqlite3-dev

echo "Установка X Window System..."
apt install -y \
    xorg \
    xserver-xorg-core \
    x11-xserver-utils \
    xinit

echo "Установка Openbox..."
apt install -y \
    openbox \
    obconf \
    obmenu \
    menu \
    xcompmgr

echo "Установка LightDM..."
apt install -y \
    lightdm \
    lightdm-gtk-greeter

echo "==============================================="
echo " Зависимости установлены!"
echo " Теперь можно сделать снимок системы (snapshot)"
echo "==============================================="