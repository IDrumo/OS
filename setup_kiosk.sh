#!/bin/bash

# Проверка на root
if [ "$EUID" -ne 0 ]; then
    echo "Этот скрипт должен запускаться с правами root (sudo)"
    exit 1
fi

echo "==============================================="
echo " Установка Weather Station Kiosk на Ubuntu 20.04"
echo "==============================================="

# 1. Обновление системы
echo "1. Обновление системы..."
apt update -y
apt upgrade -y

# 2. Установка необходимых пакетов
echo "2. Установка пакетов..."
apt install -y \
    lightdm \
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
    qt5-network \
    net-tools \
    curl \
    wget \
    unclutter

# 3. Создание пользователя для киоска
echo "3. Создание пользователя weather..."
if id "weather" &>/dev/null; then
    echo "Пользователь weather уже существует"
else
    useradd -m -s /bin/bash weather
    echo "weather:weather123" | chpasswd
    usermod -a -G sudo weather
    echo "Пользователь weather создан"
fi

# 4. Настройка LightDM для автовхода
echo "4. Настройка LightDM..."
cat > /etc/lightdm/lightdm.conf << 'EOF'
[Seat:*]
autologin-user=weather
autologin-user-timeout=0
greeter-session=lightdm-gtk-greeter
user-session=openbox
EOF

# 5. Клонирование и подготовка проекта
echo "5. Клонирование проекта..."
cd /home/weather

if [ -d "OS" ]; then
    echo "Репозиторий уже склонирован, обновляем..."
    cd OS
    git pull origin main
else
    echo "Клонирование репозитория..."
    sudo -u weather git clone https://github.com/IDrumo/OS.git
    cd OS
fi

# 6. Поиск проекта Kiosk
echo "6. Поиск проекта '7. Kiosk'..."
KIOSK_DIR=""

# Ищем директорию с проектом Kiosk
for dir in *; do
    if [ -d "$dir" ] && [[ "$dir" == *"Kiosk"* || "$dir" == *"kiosk"* ]]; then
        KIOSK_DIR="$dir"
        echo "Найден проект: $KIOSK_DIR"
        break
    fi
done

if [ -z "$KIOSK_DIR" ]; then
    # Если не нашли по имени, ищем любую папку с CMakeLists.txt
    for dir in *; do
        if [ -d "$dir" ] && [ -f "$dir/CMakeLists.txt" ]; then
            KIOSK_DIR="$dir"
            echo "Найден проект с CMakeLists.txt: $KIOSK_DIR"
            break
        fi
    done
fi

if [ -z "$KIOSK_DIR" ]; then
    echo "ОШИБКА: Не найден проект Kiosk в репозитории!"
    echo "Содержимое репозитория:"
    ls -la
    exit 1
fi

# 7. Сборка проекта
echo "7. Сборка проекта в директории: $KIOSK_DIR"
cd "$KIOSK_DIR"

# Даем права на все файлы
chown -R weather:weather .
sudo -u weather chmod -R 755 .

# Создаем директорию сборки
sudo -u weather mkdir -p build
cd build

# Конфигурация CMake
echo "Конфигурация CMake..."
sudo -u weather cmake ..

# Сборка
echo "Сборка проекта..."
sudo -u weather make -j$(nproc)

# Проверка собранных файлов
echo "Проверка собранных файлов:"
ls -la *.exe 2>/dev/null || ls -la 2>/dev/null

# 8. Создание конфигурационных файлов
echo "8. Создание конфигурационных файлов..."

# 8.1 .xinitrc
cat > /home/weather/.xinitrc << 'EOF'
#!/bin/bash

# Отключение энергосбережения
xset -dpms
xset s off
xset s noblank

# Блокировка системных клавиш
xmodmap -e "keycode 115 ="  # Alt+Tab
xmodmap -e "keycode 64 ="   # Alt
xmodmap -e "keycode 133 ="  # Super/Win
xmodmap -e "keycode 37 ="   # Ctrl
xmodmap -e "keycode 107 ="  # Print Screen

# Убираем курсор через 3 секунды бездействия (можно закомментировать)
# unclutter -idle 3 -root &

# Переходим в директорию с проектом
cd /home/weather/OS/'"$KIOSK_DIR"'/build

# Убиваем старые процессы если есть
pkill -f sensor_simulator 2>/dev/null
pkill -f weather_server 2>/dev/null
pkill -f weather_gui 2>/dev/null

# Запуск симулятора и сервера в фоне
echo "Запуск sensor_simulator..."
./sensor_simulator &
SIM_PID=$!

sleep 1

echo "Запуск weather_server..."
./weather_server virtual_com &
SERVER_PID=$!

sleep 2

# Запуск GUI
echo "Запуск weather_gui..."
./weather_gui

# Если GUI закрылся, убиваем все процессы
echo "GUI закрыт. Остановка процессов..."
kill $SIM_PID 2>/dev/null
kill $SERVER_PID 2>/dev/null
pkill -f sensor_simulator 2>/dev/null
pkill -f weather_server 2>/dev/null
pkill -f weather_gui 2>/dev/null

# Перезагрузка системы
echo "Перезагрузка системы..."
sleep 2
sudo reboot
EOF

chmod +x /home/weather/.xinitrc
chown weather:weather /home/weather/.xinitrc

# 8.2 Настройка Openbox
mkdir -p /home/weather/.config/openbox
cat > /home/weather/.config/openbox/rc.xml << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<openbox_config xmlns="http://openbox.org/3.4/rc">
  <keyboard>
    <!-- Блокируем все системные сочетания клавиш -->
    <keybind key="A-F4">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
    <keybind key="A-Tab">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
    <keybind key="C-A-Delete">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
    <keybind key="C-A-F1">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
    <keybind key="C-A-F7">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
  </keyboard>

  <mouse>
    <context name="Root">
      <!-- Блокируем контекстное меню на рабочем столе -->
      <mousebind button="Right" action="Press">
        <action name="Execute">
          <command>true</command>
        </action>
      </mousebind>
    </context>
  </mouse>
</openbox_config>
EOF

chown -R weather:weather /home/weather/.config

# 9. Настройка systemd службы
echo "9. Настройка systemd службы..."

cat > /etc/systemd/system/weather-kiosk.service << 'EOF'
[Unit]
Description=Weather Station Kiosk Mode
After=graphical.target network.target
Wants=graphical.target

[Service]
User=weather
Environment=DISPLAY=:0
Environment=XAUTHORITY=/home/weather/.Xauthority
Type=simple
ExecStart=/usr/bin/startx /etc/X11/Xsession /home/weather/.xinitrc -- :0 vt7
Restart=always
RestartSec=10
TimeoutStopSec=5

# Жесткое ограничение ресурсов
LimitNOFILE=65536
LimitNPROC=512

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable weather-kiosk.service

# 10. Блокировка системных функций
echo "10. Блокировка системных функций..."

# Отключить виртуальные терминалы
echo "NAutoVTs=0" >> /etc/systemd/logind.conf
echo "ReserveVT=0" >> /etc/systemd/logind.conf

# Заблокировать Ctrl+Alt+Del
systemctl mask ctrl-alt-del.target

# Отключить спящий режим
systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target

# Отключить менеджер обновлений
systemctl disable apt-daily.timer
systemctl disable apt-daily-upgrade.timer

# 11. Настройка GRUB для скрытия загрузки
echo "11. Настройка GRUB..."
sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT=".*"/GRUB_CMDLINE_LINUX_DEFAULT="quiet splash"/g' /etc/default/grub
echo "GRUB_TIMEOUT=0" >> /etc/default/grub
update-grub

# 12. Настройка прав sudo для пользователя weather
echo "12. Настройка прав sudo..."
echo "weather ALL=(ALL) NOPASSWD: /sbin/reboot, /sbin/shutdown, /bin/systemctl" >> /etc/sudoers.d/weather
chmod 440 /etc/sudoers.d/weather

# 13. Финальная проверка
echo "13. Финальная проверка..."

# Проверка собранных файлов
echo "Проверка собранных файлов:"
cd /home/weather/OS/"$KIOSK_DIR"/build
ls -la | grep -E "(weather_|sensor_)"

# Проверка службы
systemctl status weather-kiosk.service --no-pager

# 14. Информация для пользователя
echo "==============================================="
echo " УСТАНОВКА ЗАВЕРШЕНА!"
echo "==============================================="
echo "Информация:"
echo "- Пользователь: weather"
echo "- Пароль: weather123"
echo "- Автологин: включен"
echo "- Запуск приложения: при загрузке системы"
echo "- Секретный выход из приложения: Ctrl+Shift+Q"
echo "- Для выхода из киоск-режима: перезагрузить систему"
echo "==============================================="
echo "Следующие шаги:"
echo "1. Перезагрузите систему: sudo reboot"
echo "2. После перезагрузки система автоматически запустит Weather Station"
echo "3. Для проверки: попробуйте Alt+F4, Alt+Tab, Ctrl+Alt+Del"
echo "==============================================="

# Перезагрузка (опционально)
read -p "Перезагрузить систему сейчас? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Перезагрузка..."
    reboot
fi