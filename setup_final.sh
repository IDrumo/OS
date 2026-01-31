#!/bin/bash
# setup_final.sh - полная настройка киоска

if [ "$EUID" -ne 0 ]; then
    echo "Запустите с sudo: sudo ./setup_final.sh"
    exit 1
fi

echo "==============================================="
echo " Финальная настройка Weather Station Kiosk"
echo "==============================================="

# 1. Установите, если еще не установлены
echo "1. Проверка пакетов..."
apt install -y lightdm openbox xinit --no-install-recommends

# 2. Настройка LightDM
echo "2. Настройка LightDM..."
mkdir -p /etc/lightdm/lightdm.conf.d
cat > /etc/lightdm/lightdm.conf.d/50-kiosk.conf << 'EOF'
[Seat:*]
autologin-user=kiosk
autologin-user-timeout=0
greeter-session=lightdm-gtk-greeter
user-session=openbox
EOF

# 3. Настройка Openbox для пользователя kiosk
echo "3. Настройка Openbox..."
mkdir -p /home/kiosk/.config/openbox

# Создаем минимальный rc.xml
cat > /home/kiosk/.config/openbox/rc.xml << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<openbox_config xmlns="http://openbox.org/3.4/rc">
  <keyboard>
    <keybind key="A-F4"><action name="Execute"><command>true</command></action></keybind>
    <keybind key="A-Tab"><action name="Execute"><command>true</command></action></keybind>
    <keybind key="C-A-Delete"><action name="Execute"><command>true</command></action></keybind>
  </keyboard>
</openbox_config>
EOF

# Создаем autostart
cat > /home/kiosk/.config/openbox/autostart << 'EOF'
#!/bin/bash
# Ждем
sleep 1
# Запускаем приложение
cd "/home/kiosk/OS/7. Kiosk/build"
./sensor_simulator &
sleep 1
./weather_server virtual_com &
sleep 2
exec ./weather_gui
EOF

chmod +x /home/kiosk/.config/openbox/autostart
chown -R kiosk:kiosk /home/kiosk/.config

# 4. Создаем .xinitrc для совместимости
echo "4. Создание .xinitrc..."
cat > /home/kiosk/.xinitrc << 'EOF'
#!/bin/bash
exec openbox-session
EOF
chmod +x /home/kiosk/.xinitrc
chown kiosk:kiosk /home/kiosk/.xinitrc

# 5. Настройка прав для перезагрузки
echo "5. Настройка прав..."
echo "kiosk ALL=(ALL) NOPASSWD: /sbin/reboot" > /etc/sudoers.d/kiosk
chmod 440 /etc/sudoers.d/kiosk

# 6. Включение LightDM
echo "6. Включение LightDM..."
systemctl enable lightdm
systemctl set-default graphical.target

# 7. Отключение ненужных сервисов
echo "7. Отключение системных функций..."
systemctl mask ctrl-alt-del.target 2>/dev/null || true

echo "==============================================="
echo " ГОТОВО!"
echo "==============================================="
echo "1. Для перезагрузки: sudo reboot"
echo "2. После перезагрузки система автоматически:"
echo "   - Запустит LightDM"
echo "   - Автоматически войдет как kiosk"
echo "   - Запустит Openbox"
echo "   - Запустит Weather Station в полноэкранном режиме"
echo ""
echo "Для тестирования блокировки:"
echo "- Alt+F4: не должен закрывать"
echo "- Alt+Tab: не должен работать"
echo "- Ctrl+Alt+Del: не должен работать"
echo "- Ctrl+Shift+Q: должен закрыть приложение"
echo "==============================================="

# Перезагрузка
read -p "Перезагрузить сейчас? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Перезагрузка через 5 секунд..."
    sleep 5
    reboot
fi