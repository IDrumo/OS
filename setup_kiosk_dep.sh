#!/bin/bash
# save_as: setup_kiosk_fixed.sh

if [ "$EUID" -ne 0 ]; then
    echo "Запустите с sudo: sudo ./setup_kiosk_fixed.sh"
    exit 1
fi

echo "==============================================="
echo " Установка Weather Station Kiosk (исправленная)"
echo "==============================================="

# 1. Обновление системы
echo "1. Обновление системы..."
apt update -y
apt upgrade -y

# 2. Установка пакетов
echo "2. Установка пакетов..."
apt install -y lightdm lightdm-gtk-greeter openbox xorg  xserver-xorg-core  x11-xserver-utils  build-essential  cmake  git  sqlite3  libsqlite3-dev  qt5-default  qtbase5-dev  qtcharts5-dev  libqt5charts5-dev  libqt5network5  net-tools  curl  wget