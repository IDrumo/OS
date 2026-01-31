#!/bin/bash
# setup_kiosk.sh - только настройка киоска (предполагает, что пакеты уже установлены)

if [ "$EUID" -ne 0 ]; then
    echo "Запустите с sudo: sudo ./setup_kiosk.sh"
    exit 1
fi

echo "==============================================="
echo " Настройка Weather Station Kiosk"
echo "==============================================="

# 1. Создание пользователя для киоска
echo "1. Создание пользователя kiosk..."
if id "kiosk" &>/dev/null; then
    echo "Пользователь kiosk уже существует"
else
    useradd -m -s /bin/bash kiosk
    echo "kiosk:password" | chpasswd
    echo "Пользователь kiosk создан"
fi

# 2. Настройка LightDM для автологина
echo "2. Настройка LightDM..."
systemctl stop lightdm 2>/dev/null

# Создаем конфигурационный файл для LightDM
mkdir -p /etc/lightdm/lightdm.conf.d
cat > /etc/lightdm/lightdm.conf.d/50-kiosk.conf << 'EOF'
[Seat:*]
autologin-user=kiosk
autologin-user-timeout=0
greeter-session=lightdm-gtk-greeter
user-session=openbox
EOF

# 3. Клонирование проекта
echo "3. Клонирование проекта..."
cd /home/kiosk
if [ ! -d "OS" ]; then
    sudo -u kiosk git clone https://github.com/IDrumo/OS.git
fi

# 4. Поиск проекта Kiosk
echo "4. Поиск проекта Kiosk..."
cd OS

# Ищем папку с проектом Kiosk
KIOSK_PROJECT=$(find . -maxdepth 1 -type d \( -name "*[Kk]iosk*" -o -name "*7*" \) | head -1)
if [ -z "$KIOSK_PROJECT" ]; then
    # Если не нашли, берем первую папку с CMakeLists.txt
    KIOSK_PROJECT=$(find . -maxdepth 1 -type d -exec test -f "{}/CMakeLists.txt" \; -print | head -1)
fi

if [ -z "$KIOSK_PROJECT" ]; then
    echo "ОШИБКА: Не найден проект Kiosk!"
    exit 1
fi

KIOSK_PROJECT=${KIOSK_PROJECT#./}
echo "Найден проект: $KIOSK_PROJECT"

# 5. Сборка проекта
echo "5. Сборка проекта..."
cd "$KIOSK_PROJECT"
chown -R kiosk:kiosk .
sudo -u kiosk mkdir -p build
cd build

# Очистка предыдущей сборки
sudo -u kiosk rm -rf *

# Конфигурация и сборка
echo "Конфигурация CMake..."
sudo -u kiosk cmake ..
echo "Сборка..."
sudo -u kiosk make -j$(nproc)

# 6. Создание скрипта запуска (оркестратора)
echo "6. Создание скрипта запуска..."
cat > /home/kiosk/kiosk_launch.sh << 'EOF'
#!/bin/bash
# Оркестратор для киоска

# Функция для установки разрешения экрана
set_resolution() {
    if command -v xrandr &> /dev/null; then
        # Пробуем установить разрешение 1024x768
        xrandr --output Virtual1 --mode 1024x768 2>/dev/null || \
        xrandr --output default --mode 1024x768 2>/dev/null || true
    fi
}

# Функция для блокировки клавиш
lock_keys() {
    # Отключаем Alt, Super, Ctrl+Alt+Del и другие системные комбинации
    xmodmap -e "keycode 64 =" 2>/dev/null  # Alt
    xmodmap -e "keycode 133 =" 2>/dev/null # Super
    xmodmap -e "keycode 107 =" 2>/dev/null # Print Screen
    xmodmap -e "keycode 78 =" 2>/dev/null  # Scroll Lock
    true
}

# Установка разрешения
set_resolution

# Блокировка клавиш
lock_keys

# Переходим в директорию с проектом
cd /home/kiosk/OS/'"$KIOSK_PROJECT"'/build

# Бесконечный цикл для отказоустойчивости
while true; do
    echo "$(date): Запуск Weather Station..."

    # Убиваем старые процессы если есть
    pkill -f sensor_simulator 2>/dev/null
    pkill -f weather_server 2>/dev/null
    pkill -f weather_gui 2>/dev/null

    # Запускаем симулятор
    ./sensor_simulator &
    SIM_PID=$!

    sleep 1

    # Запускаем сервер
    ./weather_server virtual_com &
    SERVER_PID=$!

    sleep 2

    # Запускаем GUI
    echo "GUI запущен. Для выхода нажмите Ctrl+Shift+Q"
    ./weather_gui

    # Если GUI закрылся
    echo "GUI закрыт, перезапуск через 2 секунды..."
    kill $SIM_PID 2>/dev/null
    kill $SERVER_PID 2>/dev/null
    sleep 2
done
EOF

chmod +x /home/kiosk/kiosk_launch.sh
chown kiosk:kiosk /home/kiosk/kiosk_launch.sh

echo "7. Настройка Openbox..."

mkdir -p /home/kiosk/.config/openbox

cat > /home/kiosk/.config/openbox/rc.xml << 'EOF'
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
    <keybind key="C-A-F2">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
    <keybind key="C-A-F3">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
    <keybind key="C-A-F4">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
    <keybind key="C-A-F5">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
    <keybind key="C-A-F6">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
    <keybind key="C-A-F7">
      <action name="Execute">
        <command>true</command>
      </action>
    </keybind>
    <keybind key="W">
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

  <!-- Убираем декорации окон -->
  <theme>
    <name>Clearlooks</name>
    <titleLayout>NLIMC</titleLayout>
    <keepBorder>yes</keepBorder>
    <animateIconify>no</animateIconify>
    <font place="ActiveWindow">
      <name>sans</name>
      <size>10</size>
    </font>
  </theme>
</openbox_config>
EOF

cat > /home/kiosk/.config/openbox/autostart << 'EOF'
#!/bin/bash

xset -dpms
xset s off
xset s noblank

# unclutter -idle 3 -root &

xcompmgr -c -f -D 5 &

sleep 2

exec /home/kiosk/kiosk_launch.sh
EOF

chmod +x /home/kiosk/.config/openbox/autostart
chown -R kiosk:kiosk /home/kiosk/.config

cat > /home/kiosk/.xsession << 'EOF'
#!/bin/bash
exec openbox-session
EOF
chmod +x /home/kiosk/.xsession
chown kiosk:kiosk /home/kiosk/.xsession

echo "8. Создание сессии Openbox..."
cat > /usr/share/xsessions/openbox-kiosk.desktop << 'EOF'
[Desktop Entry]
Name=Openbox Kiosk
Comment=Openbox with Weather Station Kiosk
Exec=openbox-session
TryExec=openbox-session
Type=Application
EOF

echo "9. Настройка прав sudo..."
echo "kiosk ALL=(ALL) NOPASSWD: /sbin/reboot, /sbin/shutdown, /usr/bin/pkill" > /etc/sudoers.d/kiosk
chmod 440 /etc/sudoers.d/kiosk

echo "10. Отключение системных функций..."
systemctl mask ctrl-alt-del.target 2>/dev/null || true
systemctl mask sleep.target suspend.target hibernate.target 2>/dev/null || true

echo "11. Включение LightDM..."
systemctl enable lightdm

echo "==============================================="
echo " НАСТРОЙКА ЗАВЕРШЕНА!"
echo "==============================================="
echo "Информация:"
echo "- Пользователь: kiosk"
echo "- Пароль: password"
echo "- Автологин: включен"
echo "- Оконный менеджер: Openbox"
echo "- Запуск: при загрузке автоматически"
echo "- Отказоустойчивость: приложение перезапускается при падении"
echo "- Выход из приложения: Ctrl+Shift+Q (только внутри приложения)"
echo "==============================================="
echo "Для перезагрузки выполните: sudo reboot"
echo "==============================================="