#include "temp_monitor_gui.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QStyleFactory>
#include <QFile>
#include <QTextStream>
#include <QCommandLineParser>
#include <QSharedMemory>

// Макрос для кроссплатформенного выхода
#ifdef _WIN32
    #include <windows.h>
    #define APP_EXIT(code) ExitProcess(code)
#else
    #include <stdlib.h>
    #define APP_EXIT(code) exit(code)
#endif

// Проверка на уже запущенный экземпляр
bool isAlreadyRunning(const QString& appName)
{
    static QSharedMemory sharedMemory;
    sharedMemory.setKey(appName);
    
    if (sharedMemory.attach()) {
        return true; // Уже запущен
    }
    
    if (!sharedMemory.create(1)) {
        // Ошибка создания shared memory
        return true;
    }
    
    return false;
}

// Инициализация стиля приложения
void setupApplicationStyle(QApplication& app)
{
    // Устанавливаем тему в зависимости от системы
    #ifdef _WIN32
        app.setStyle(QStyleFactory::create("Fusion"));
        
        // Темная тема для Windows 10/11
        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 
                          QSettings::NativeFormat);
        if (settings.value("AppsUseLightTheme") == 0) {
            QPalette darkPalette;
            darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
            darkPalette.setColor(QPalette::WindowText, Qt::white);
            darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
            darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
            darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
            darkPalette.setColor(QPalette::ToolTipText, Qt::white);
            darkPalette.setColor(QPalette::Text, Qt::white);
            darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
            darkPalette.setColor(QPalette::ButtonText, Qt::white);
            darkPalette.setColor(QPalette::BrightText, Qt::red);
            darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
            darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
            darkPalette.setColor(QPalette::HighlightedText, Qt::black);
            app.setPalette(darkPalette);
        }
    #else
        // Для Linux используем системный стиль
        QStringList styles = QStyleFactory::keys();
        if (styles.contains("Breeze")) {
            app.setStyle("Breeze");
        } else if (styles.contains("Fusion")) {
            app.setStyle("Fusion");
        }
    #endif
    
    // Загружаем стили из файла если существует
    QFile styleFile(":/styles/style.qss");
    if (styleFile.exists()) {
        styleFile.open(QFile::ReadOnly);
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
    }
}

int main(int argc, char *argv[])
{
    // Настройка для высокого DPI
    #if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    #endif
    
    QApplication app(argc, argv);
    
    // Проверяем, не запущено ли уже приложение
    if (isAlreadyRunning("TemperatureMonitorGUI")) {
        QMessageBox::warning(nullptr, 
                           QObject::tr("Предупреждение"),
                           QObject::tr("Приложение уже запущено!"));
        APP_EXIT(1);
    }
    
    // Настройка информации о приложении
    app.setApplicationName("TemperatureMonitorGUI");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("TemperatureMonitor");
    app.setOrganizationDomain("temperature-monitor.local");
    
    // Парсинг аргументов командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("GUI монитор температуры"));
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption serverOption({"s", "server"}, 
                                   QObject::tr("URL сервера"), 
                                   "url", 
                                   "http://localhost:8080");
    parser.addOption(serverOption);
    
    QCommandLineOption portOption({"p", "port"}, 
                                 QObject::tr("Порт сервера"), 
                                 "port", 
                                 "8080");
    parser.addOption(portOption);
    
    parser.process(app);
    
    // Настройка локализации
    QTranslator translator;
    QString locale = QLocale::system().name();
    
    #ifdef _WIN32
        if (translator.load(":/translations/tempmonitor_" + locale + ".qm")) {
            app.installTranslator(&translator);
        }
    #else
        // Для Linux ищем переводы в стандартных местах
        QStringList translationPaths = {
            "/usr/share/temperature-monitor/translations",
            QApplication::applicationDirPath() + "/../share/temperature-monitor/translations",
            QApplication::applicationDirPath() + "/translations"
        };
        
        for (const QString& path : translationPaths) {
            if (translator.load("tempmonitor_" + locale, path)) {
                app.installTranslator(&translator);
                break;
            }
        }
    #endif
    
    // Настройка стиля приложения
    setupApplicationStyle(app);
    
    try {
        // Создаем и показываем главное окно
        TempMonitorGUI window;
        
        // Применяем параметры командной строки
        if (parser.isSet(serverOption)) {
            QString serverUrl = parser.value(serverOption);
            // Здесь можно передать URL в окно, если добавить соответствующий метод
        }
        
        window.show();
        
        return app.exec();
        
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, 
                            QObject::tr("Критическая ошибка"),
                            QObject::tr("Ошибка запуска приложения: %1").arg(e.what()));
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, 
                            QObject::tr("Критическая ошибка"),
                            QObject::tr("Неизвестная ошибка запуска приложения"));
        return -1;
    }
}