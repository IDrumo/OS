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
apt install -y \
    lightdm \
    lightdm-gtk-greeter \
    openbox \
    xorg \
    xserver-xorg-core \
    x11-xserver-utils \
    build-essential \
    cmake \
    git \
    sqlite3 \
    libsqlite3-dev \
    qt5-default \
    qtbase5-dev \
    qtcharts5-dev \
    libqt5charts5-dev \
    libqt5network5 \
    net-tools \
    curl \
    wget

# 3. Создание пользователя
echo "3. Создание пользователя..."
useradd -m -s /bin/bash weather 2>/dev/null || true
echo "weather:weather123" | chpasswd

# 4. Настройка LightDM
echo "4. Настройка LightDM..."
systemctl stop lightdm 2>/dev/null || true

cat > /etc/lightdm/lightdm.conf.d/50-weather.conf << 'EOF'
[Seat:*]
autologin-user=weather
autologin-user-timeout=0
greeter-session=lightdm-gtk-greeter
user-session=openbox
EOF

# 5. Клонирование проекта
echo "5. Клонирование проекта..."
cd /home/weather
if [ ! -d "OS" ]; then
    sudo -u weather git clone https://github.com/IDrumo/OS.git
fi

# 6. Поиск проекта Kiosk
echo "6. Поиск проекта Kiosk..."
cd OS
KIOSK_DIR=$(find . -maxdepth 1 -type d \( -name "*[Kk]iosk*" -o -name "*7*" \) | head -1)
if [ -z "$KIOSK_DIR" ]; then
    KIOSK_DIR=$(find . -maxdepth 1 -type d | grep -v "^\.$" | head -1)
fi

if [ -z "$KIOSK_DIR" ]; then
    echo "Ошибка: Не найден проект!"
    exit 1
fi

KIOSK_DIR=${KIOSK_DIR#./}
echo "Найден проект: $KIOSK_DIR"

# 7. Сборка проекта
echo "7. Сборка проекта..."
cd "$KIOSK_DIR"
chown -R weather:weather .
sudo -u weather mkdir -p build
cd build
sudo -u weather cmake ..
sudo -u weather make -j$(nproc)

# 8. Создание скрипта запуска
echo "8. Создание скрипта запуска..."
cat > /home/weather/start_weather.sh << 'EOF'
#!/bin/bash
cd /home/weather/OS/'"$KIOSK_DIR"'/build

# Запуск компонентов
./sensor_simulator &
sleep 1
./weather_server virtual_com &
sleep 2

# Запуск GUI
exec ./weather_gui
EOF

chmod +x /home/weather/start_weather.sh
chown weather:weather /home/weather/start_weather.sh

# 9. Настройка Openbox autostart
echo "9. Настройка Openbox..."
mkdir -p /home/weather/.config/openbox

cat > /home/weather/.config/openbox/autostart << 'EOF'
#!/bin/bash

# Дать системе время на инициализацию
sleep 2

# Отключить энергосбережение
xset -dpms
xset s off
xset s noblank

# Запустить приложение
bash /home/weather/start_weather.sh

# Если приложение закрылось
echo "Перезагрузка системы..."
sleep 2
sudo reboot
EOF

chmod +x /home/weather/.config/openbox/autostart
chown -R weather:weather /home/weather/.config

# 10. Настройка .xinitrc (на всякий случай)
cat > /home/weather/.xinitrc << 'EOF'
#!/bin/bash
exec openbox-session
EOF
chmod +x /home/weather/.xinitrc

# 11. Настройка прав для перезагрузки
echo "10. Настройка прав..."
echo "weather ALL=(ALL) NOPASSWD: /sbin/reboot, /sbin/shutdown" > /etc/sudoers.d/weather

# 12. Блокировка системных функций
echo "11. Блокировка системных функций..."
systemctl mask ctrl-alt-del.target 2>/dev/null || true
systemctl mask sleep.target suspend.target hibernate.target 2>/dev/null || true

# 13. Включение LightDM
echo "12. Включение LightDM..."
systemctl enable lightdm

# 14. Финальные шаги
echo "==============================================="
echo " УСТАНОВКА ЗАВЕРШЕНА!"
echo "==============================================="
echo "Проверка:"
echo "1. Проверьте, что файлы собраны:"
ls -la /home/weather/OS/"$KIOSK_DIR"/build/
echo ""
echo "2. Для перезагрузки выполните: sudo reboot"
echo "3. После перезагрузки должно запуститься приложение"
echo "4. Для выхода из приложения: Ctrl+Shift+Q"
echo "==============================================="

# 15. Перезагрузка
read -p "Перезагрузить сейчас? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Перезагрузка..."
    reboot
fi