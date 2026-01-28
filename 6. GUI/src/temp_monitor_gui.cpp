#include "temp_monitor_gui.h"

// QWT заголовки
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_text.h>

// Стандартные заголовки
#include <QCloseEvent>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QDesktopWidget>
#include <QStyleFactory>
#include <QScreen>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <sys/stat.h>
    #include <pwd.h>
    #include <unistd.h>
#endif

// Макрос для безопасного удаления
#define SAFE_DELETE(ptr) { if(ptr) { delete ptr; ptr = nullptr; } }

// Макрос для проверки указателя
#define CHECK_PTR(ptr, action) { if(!ptr) { qWarning() << "Null pointer:" << #ptr; action; } }

// Макрос для логирования
#ifdef QT_DEBUG
    #define DEBUG_LOG(msg) qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << msg
#else
    #define DEBUG_LOG(msg)
#endif

TemperatureDataFetcher::TemperatureDataFetcher(QObject *parent)
    : QObject(parent)
{
}

void TemperatureDataFetcher::fetchData(const QString& url, bool realtime)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "TemperatureMonitor/1.0");
    
    QNetworkReply *reply = manager.get(request);
    connect(reply, &QNetworkReply::finished, 
            this, [this, reply, realtime]() { onReplyFinished(reply); });
    
    DEBUG_LOG("Fetching data from:" << url);
}

void TemperatureDataFetcher::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) {
        emit error("Invalid JSON response");
        return;
    }
    
    QJsonObject obj = doc.object();
    QJsonArray dataArray = obj["data"].toArray();
    
    emit dataReady(dataArray, false);
}

// ========== TempMonitorGUI Implementation ==========

TempMonitorGUI::TempMonitorGUI(QWidget *parent)
    : QMainWindow(parent)
    , isRealtimeMode(false)
    , currentPeriod(0)
    , serverUrl("http://localhost:8080/api/range")
{
    setupUI();
    setupChart();
    setupConnections();
    loadSettings();
    
    // Начальная загрузка данных
    QTimer::singleShot(100, this, &TempMonitorGUI::onFetchData);
}

TempMonitorGUI::~TempMonitorGUI()
{
    saveSettings();
    SAFE_DELETE(fetcher);
}

void TempMonitorGUI::setupUI()
{
    // Центральный виджет
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    mainLayout = new QVBoxLayout(centralWidget);
    
    // Устанавливаем иконку окна
    #ifdef _WIN32
        setWindowIcon(QIcon(":/icons/temperature.ico"));
    #else
        setWindowIcon(QIcon(":/icons/temperature.png"));
    #endif
    
    // Группа управления
    controlGroup = new QGroupBox(TEMP_TEXT("Управление"), centralWidget);
    controlLayout = new QHBoxLayout(controlGroup);
    
    // Выбор периода
    periodCombo = new QComboBox(controlGroup);
    periodCombo->addItem(TEMP_TEXT("1 час"), 1);
    periodCombo->addItem(TEMP_TEXT("6 часов"), 6);
    periodCombo->addItem(TEMP_TEXT("24 часа"), 24);
    periodCombo->addItem(TEMP_TEXT("Неделя"), 168);
    periodCombo->addItem(TEMP_TEXT("Месяц"), 720);
    
    // Временной интервал
    updateIntervalSpin = new QSpinBox(controlGroup);
    updateIntervalSpin->setRange(1, 3600);
    updateIntervalSpin->setValue(10);
    updateIntervalSpin->setSuffix(TEMP_TEXT(" сек"));
    
    // Кнопки
    fetchButton = new QPushButton(TEMP_TEXT("Обновить"), controlGroup);
    realtimeButton = new QPushButton(TEMP_TEXT("Реальное время"), controlGroup);
    realtimeButton->setCheckable(true);
    
    saveButton = new QPushButton(TEMP_TEXT("Сохранить"), controlGroup);
    loadButton = new QPushButton(TEMP_TEXT("Загрузить"), controlGroup);
    clearButton = new QPushButton(TEMP_TEXT("Очистить"), controlGroup);
    statsButton = new QPushButton(TEMP_TEXT("Статистика"), controlGroup);
    
    // Автообновление
    autoUpdateCheck = new QCheckBox(TEMP_TEXT("Автообновление"), controlGroup);
    
    // Добавляем элементы в layout управления
    controlLayout->addWidget(new QLabel(TEMP_TEXT("Период:"), controlGroup));
    controlLayout->addWidget(periodCombo);
    controlLayout->addWidget(new QLabel(TEMP_TEXT("Интервал:"), controlGroup));
    controlLayout->addWidget(updateIntervalSpin);
    controlLayout->addWidget(fetchButton);
    controlLayout->addWidget(realtimeButton);
    controlLayout->addWidget(autoUpdateCheck);
    controlLayout->addStretch();
    controlLayout->addWidget(saveButton);
    controlLayout->addWidget(loadButton);
    controlLayout->addWidget(clearButton);
    controlLayout->addWidget(statsButton);
    
    // Группа времени
    QGroupBox *timeGroup = new QGroupBox(TEMP_TEXT("Время"), centralWidget);
    timeLayout = new QHBoxLayout(timeGroup);
    
    currentTimeLabel = new QLabel(TEMP_TEXT("Текущее время: --:--:--"), timeGroup);
    QLabel *serverLabel = new QLabel(TEMP_TEXT("Сервер: localhost:8080"), timeGroup);
    
    timeLayout->addWidget(currentTimeLabel);
    timeLayout->addStretch();
    timeLayout->addWidget(serverLabel);
    
    // Группа графика
    chartGroup = new QGroupBox(TEMP_TEXT("График температуры"), centralWidget);
    chartLayout = new QHBoxLayout(chartGroup);
    
    // Создаем график Qwt
    plot = new QwtPlot(chartGroup);
    plot->setMinimumHeight(400);
    chartLayout->addWidget(plot);
    
    // Группа статистики
    statsGroup = new QGroupBox(TEMP_TEXT("Статистика"), centralWidget);
    statsLayout = new QHBoxLayout(statsGroup);
    
    currentTempLabel = new QLabel(TEMP_TEXT("Текущая: --.-°C"), statsGroup);
    avgTempLabel = new QLabel(TEMP_TEXT("Средняя: --.-°C"), statsGroup);
    minTempLabel = new QLabel(TEMP_TEXT("Минимум: --.-°C"), statsGroup);
    maxTempLabel = new QLabel(TEMP_TEXT("Максимум: --.-°C"), statsGroup);
    
    currentTempLabel->setStyleSheet("QLabel { font-weight: bold; color: #2196F3; }");
    avgTempLabel->setStyleSheet("QLabel { color: #4CAF50; }");
    minTempLabel->setStyleSheet("QLabel { color: #FF9800; }");
    maxTempLabel->setStyleSheet("QLabel { color: #F44336; }");
    
    statsLayout->addWidget(currentTempLabel);
    statsLayout->addWidget(avgTempLabel);
    statsLayout->addWidget(minTempLabel);
    statsLayout->addWidget(maxTempLabel);
    statsLayout->addStretch();
    
    // Добавляем все группы в основной layout
    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(timeGroup);
    mainLayout->addWidget(chartGroup, 1); // Растягиваем график
    mainLayout->addWidget(statsGroup);
    
    // Статус бар
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    progressBar = new QProgressBar(statusBar);
    progressBar->setMaximumWidth(200);
    progressBar->setVisible(false);
    statusBar->addPermanentWidget(progressBar);
    
    // Настройка таймеров
    updateTimer = new QTimer(this);
    clockTimer = new QTimer(this);
    
    // Создаем fetcher в отдельном потоке
    fetcher = std::make_unique<TemperatureDataFetcher>();
    
    // Устанавливаем заголовок окна
    setWindowTitle(TEMP_TEXT("Температурный монитор"));
    resize(1000, 700);
    
    // Центрируем окно
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        move(screenGeometry.center() - rect().center());
    }
}

void TempMonitorGUI::setupChart()
{
    // Настройка сетки
    grid = new QwtPlotGrid();
    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->setMinorPen(QPen(Qt::lightGray, 0, Qt::DotLine));
    grid->attach(plot);
    
    // Настройка кривой
    curve = new QwtPlotCurve(TEMP_TEXT("Температура"));
    curve->setPen(QPen(Qt::blue, 2));
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    
    // Символы точек
    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,
        QBrush(Qt::white), QPen(Qt::blue, 2), QSize(6, 6));
    curve->setSymbol(symbol);
    curve->attach(plot);
    
    // Маркер текущей температуры
    currentMarker = new QwtPlotMarker();
    currentMarker->setLineStyle(QwtPlotMarker::VLine);
    currentMarker->setLinePen(QPen(Qt::red, 1, Qt::DashLine));
    currentMarker->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    currentMarker->attach(plot);
    
    // Настройка осей
    plot->setAxisTitle(QwtPlot::xBottom, TEMP_TEXT("Время"));
    plot->setAxisTitle(QwtPlot::yLeft, TEMP_TEXT("Температура (°C)"));
    
    // Включаем легенду
    plot->insertLegend(new QwtLegend(), QwtPlot::RightLegend);
    
    // Включаем zoom
    plot->setAxisAutoScale(QwtPlot::xBottom, true);
    plot->setAxisAutoScale(QwtPlot::yLeft, true);
    
    // Устанавливаем формат времени
    QwtScaleDraw *scaleDraw = new QwtScaleDraw();
    scaleDraw->setDateFormat(QString("hh:mm"));
    plot->setAxisScaleDraw(QwtPlot::xBottom, scaleDraw);
}

void TempMonitorGUI::setupConnections()
{
    // Кнопки
    connect(fetchButton, &QPushButton::clicked, this, &TempMonitorGUI::onFetchData);
    connect(realtimeButton, &QPushButton::toggled, this, &TempMonitorGUI::onRealtimeToggled);
    connect(saveButton, &QPushButton::clicked, this, &TempMonitorGUI::onSaveData);
    connect(loadButton, &QPushButton::clicked, this, &TempMonitorGUI::onLoadData);
    connect(clearButton, &QPushButton::clicked, this, &TempMonitorGUI::onClearData);
    connect(statsButton, &QPushButton::clicked, this, &TempMonitorGUI::onShowStats);
    
    // Комбобокс
    connect(periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TempMonitorGUI::onPeriodChanged);
    
    // Спинбокс
    connect(updateIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TempMonitorGUI::onUpdateIntervalChanged);
    
    // Чекбокс
    connect(autoUpdateCheck, &QCheckBox::toggled, [this](bool checked) {
        if (checked) {
            updateTimer->start(updateIntervalSpin->value() * 1000);
        } else {
            updateTimer->stop();
        }
    });
    
    // Таймеры
    connect(updateTimer, &QTimer::timeout, this, &TempMonitorGUI::onFetchData);
    connect(clockTimer, &QTimer::timeout, this, &TempMonitorGUI::updateCurrentTime);
    clockTimer->start(1000);
    
    // Fetcher сигналы
    connect(fetcher.get(), &TemperatureDataFetcher::dataReady,
            this, &TempMonitorGUI::onDataFetched);
    connect(fetcher.get(), &TemperatureDataFetcher::error,
            this, &TempMonitorGUI::onFetchError);
}

void TempMonitorGUI::onFetchData()
{
    if (!fetcher) {
        QMessageBox::warning(this, TEMP_TEXT("Ошибка"), 
                           TEMP_TEXT("Ошибка инициализации fetcher"));
        return;
    }
    
    // Показываем прогресс
    progressBar->setVisible(true);
    progressBar->setRange(0, 0); // индикатор процесса
    
    // Формируем URL
    int hours = periodCombo->currentData().toInt();
    QDateTime to = QDateTime::currentDateTime();
    QDateTime from = to.addSecs(-hours * 3600);
    
    QString url = QString("%1?from=%2&to=%3")
                    .arg(serverUrl)
                    .arg(from.toUTC().toString(Qt::ISODate))
                    .arg(to.toUTC().toString(Qt::ISODate));
    
    fetcher->fetchData(url, isRealtimeMode);
}

void TempMonitorGUI::onDataFetched(const QJsonArray& data, bool isRealtime)
{
    progressBar->setVisible(false);
    
    if (data.isEmpty()) {
        statusBar->showMessage(TEMP_TEXT("Нет данных"), 3000);
        return;
    }
    
    updateChart(data);
    updateStats(data);
    
    QString msg = QString(TEMP_TEXT("Получено %1 записей"))
                    .arg(data.size());
    statusBar->showMessage(msg, 3000);
}

void TempMonitorGUI::onFetchError(const QString& error)
{
    progressBar->setVisible(false);
    
    QMessageBox::critical(this, TEMP_TEXT("Ошибка"), 
                        TEMP_TEXT("Ошибка получения данных: ") + error);
    
    if (isRealtimeMode) {
        realtimeButton->setChecked(false);
        onRealtimeToggled(false);
    }
}

void TempMonitorGUI::updateChart(const QJsonArray& data)
{
    timeData.clear();
    tempData.clear();
    
    double minTemp = std::numeric_limits<double>::max();
    double maxTemp = std::numeric_limits<double>::min();
    
    for (const QJsonValue& val : data) {
        QJsonObject obj = val.toObject();
        QString timeStr = obj["t"].toString();
        double temp = obj["v"].toDouble();
        
        QDateTime dt = QDateTime::fromString(timeStr, Qt::ISODate);
        if (dt.isValid()) {
            timeData.append(dt.toMSecsSinceEpoch() / 1000.0); // секунды
            tempData.append(temp);
            
            minTemp = qMin(minTemp, temp);
            maxTemp = qMax(maxTemp, temp);
        }
    }
    
    if (timeData.isEmpty()) return;
    
    // Обновляем кривую
    curve->setSamples(timeData, tempData);
    
    // Обновляем оси с небольшим запасом
    double timeMin = timeData.first();
    double timeMax = timeData.last();
    double timeSpan = timeMax - timeMin;
    
    plot->setAxisScale(QwtPlot::xBottom, 
                      timeMin - timeSpan * 0.02,
                      timeMax + timeSpan * 0.02);
    
    double tempSpan = maxTemp - minTemp;
    if (tempSpan < 1.0) tempSpan = 10.0; // Минимальный диапазон
    
    plot->setAxisScale(QwtPlot::yLeft,
                      minTemp - tempSpan * 0.1,
                      maxTemp + tempSpan * 0.1);
    
    // Добавляем маркер последней точки
    if (!tempData.isEmpty()) {
        currentMarker->setValue(timeData.last(), tempData.last());
        currentMarker->setLabel(QString(TEMP_TEXT("%1°C")).arg(tempData.last(), 0, 'f', 1));
    }
    
    plot->replot();
}

void TempMonitorGUI::updateStats(const QJsonArray& data)
{
    if (data.isEmpty()) return;
    
    double sum = 0;
    stats.minimum = std::numeric_limits<double>::max();
    stats.maximum = std::numeric_limits<double>::min();
    stats.count = data.size();
    
    for (const QJsonValue& val : data) {
        QJsonObject obj = val.toObject();
        double temp = obj["v"].toDouble();
        
        sum += temp;
        stats.minimum = qMin(stats.minimum, temp);
        stats.maximum = qMax(stats.maximum, temp);
    }
    
    stats.average = sum / stats.count;
    
    if (!tempData.isEmpty()) {
        stats.current = tempData.last();
    }
    
    // Обновляем labels
    currentTempLabel->setText(QString(TEMP_TEXT("Текущая: %1°C")).arg(stats.current, 0, 'f', 1));
    avgTempLabel->setText(QString(TEMP_TEXT("Средняя: %1°C")).arg(stats.average, 0, 'f', 1));
    minTempLabel->setText(QString(TEMP_TEXT("Минимум: %1°C")).arg(stats.minimum, 0, 'f', 1));
    maxTempLabel->setText(QString(TEMP_TEXT("Максимум: %1°C")).arg(stats.maximum, 0, 'f', 1));
}

void TempMonitorGUI::onRealtimeToggled(bool checked)
{
    isRealtimeMode = checked;
    
    if (checked) {
        updateTimer->start(2000); // Обновление каждые 2 секунды
        realtimeButton->setText(TEMP_TEXT("Стоп"));
        realtimeButton->setStyleSheet("background-color: #ff4444; color: white;");
    } else {
        updateTimer->stop();
        realtimeButton->setText(TEMP_TEXT("Реальное время"));
        realtimeButton->setStyleSheet("");
    }
}

void TempMonitorGUI::onPeriodChanged(int index)
{
    Q_UNUSED(index);
    if (!isRealtimeMode) {
        onFetchData();
    }
}

void TempMonitorGUI::onUpdateIntervalChanged(int value)
{
    if (autoUpdateCheck->isChecked()) {
        updateTimer->setInterval(value * 1000);
    }
}

void TempMonitorGUI::updateCurrentTime()
{
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString(TEMP_TEXT("hh:mm:ss"));
    currentTimeLabel->setText(TEMP_TEXT("Текущее время: ") + timeStr);
}

void TempMonitorGUI::onSaveData()
{
    if (tempData.isEmpty()) {
        QMessageBox::information(this, TEMP_TEXT("Информация"),
                               TEMP_TEXT("Нет данных для сохранения"));
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
        TEMP_TEXT("Сохранить данные"),
        getAppDataPath() + "/temperature_data.csv",
        TEMP_TEXT("CSV файлы (*.csv);;Все файлы (*)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, TEMP_TEXT("Ошибка"),
                            TEMP_TEXT("Не удалось открыть файл для записи"));
        return;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    // Заголовок
    out << "Timestamp,Temperature(°C)\n";
    
    // Данные
    for (int i = 0; i < timeData.size(); ++i) {
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(timeData[i] * 1000);
        out << dt.toString(Qt::ISODate) << "," 
            << QString::number(tempData[i], 'f', 2) << "\n";
    }
    
    file.close();
    statusBar->showMessage(TEMP_TEXT("Данные сохранены в: ") + fileName, 5000);
}

void TempMonitorGUI::onLoadData()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        TEMP_TEXT("Загрузить данные"),
        getAppDataPath(),
        TEMP_TEXT("CSV файлы (*.csv);;Все файлы (*)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, TEMP_TEXT("Ошибка"),
                            TEMP_TEXT("Не удалось открыть файл для чтения"));
        return;
    }
    
    QTextStream in(&file);
    in.setCodec("UTF-8");
    
    QJsonArray data;
    int lineNum = 0;
    
    // Пропускаем заголовок
    QString header = in.readLine();
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNum++;
        
        QStringList parts = line.split(',');
        if (parts.size() >= 2) {
            QDateTime dt = QDateTime::fromString(parts[0], Qt::ISODate);
            bool ok;
            double temp = parts[1].toDouble(&ok);
            
            if (dt.isValid() && ok) {
                QJsonObject obj;
                obj["t"] = dt.toString(Qt::ISODate);
                obj["v"] = temp;
                data.append(obj);
            }
        }
    }
    
    file.close();
    
    if (!data.isEmpty()) {
        updateChart(data);
        updateStats(data);
        statusBar->showMessage(QString(TEMP_TEXT("Загружено %1 записей")).arg(data.size()), 3000);
    } else {
        QMessageBox::warning(this, TEMP_TEXT("Предупреждение"),
                           TEMP_TEXT("Не удалось загрузить данные из файла"));
    }
}

void TempMonitorGUI::onClearData()
{
    if (QMessageBox::question(this, TEMP_TEXT("Подтверждение"),
                            TEMP_TEXT("Очистить все данные?"),
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        timeData.clear();
        tempData.clear();
        curve->setSamples(QVector<double>(), QVector<double>());
        plot->replot();
        
        // Сбрасываем статистику
        currentTempLabel->setText(TEMP_TEXT("Текущая: --.-°C"));
        avgTempLabel->setText(TEMP_TEXT("Средняя: --.-°C"));
        minTempLabel->setText(TEMP_TEXT("Минимум: --.-°C"));
        maxTempLabel->setText(TEMP_TEXT("Максимум: --.-°C"));
        
        statusBar->showMessage(TEMP_TEXT("Данные очищены"), 3000);
    }
}

void TempMonitorGUI::onShowStats()
{
    if (stats.count == 0) {
        QMessageBox::information(this, TEMP_TEXT("Статистика"),
                               TEMP_TEXT("Нет данных для отображения статистики"));
        return;
    }
    
    QString statsText = QString(
        TEMP_TEXT("<h3>Статистика температуры</h3>"
                 "<table border='1' cellpadding='5'>"
                 "<tr><td><b>Количество измерений:</b></td><td>%1</td></tr>"
                 "<tr><td><b>Текущая температура:</b></td><td>%2 °C</td></tr>"
                 "<tr><td><b>Средняя температура:</b></td><td>%3 °C</td></tr>"
                 "<tr><td><b>Минимальная температура:</b></td><td>%4 °C</td></tr>"
                 "<tr><td><b>Максимальная температура:</b></td><td>%5 °C</td></tr>"
                 "<tr><td><b>Диапазон:</b></td><td>%6 °C</td></tr>"
                 "</table>"))
        .arg(stats.count)
        .arg(stats.current, 0, 'f', 1)
        .arg(stats.average, 0, 'f', 1)
        .arg(stats.minimum, 0, 'f', 1)
        .arg(stats.maximum, 0, 'f', 1)
        .arg(stats.maximum - stats.minimum, 0, 'f', 1);
    
    QMessageBox::information(this, TEMP_TEXT("Детальная статистика"), statsText);
}

QString TempMonitorGUI::getAppDataPath() const
{
    QString path;
    
    #ifdef _WIN32
        wchar_t* appData = nullptr;
        if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appData) == S_OK) {
            path = QString::fromWCharArray(appData) + "/TemperatureMonitor";
            CoTaskMemFree(appData);
        }
    #else
        const char* homeDir = getenv("HOME");
        if (homeDir) {
            path = QString(homeDir) + "/.temperature-monitor";
        }
    #endif
    
    if (!path.isEmpty()) {
        QDir dir(path);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }
    
    return path;
}

QString TempMonitorGUI::formatDateTime(const QDateTime& dt) const
{
    #ifdef _WIN32
        return dt.toString("dd.MM.yyyy hh:mm:ss");
    #else
        return dt.toString("yyyy-MM-dd hh:mm:ss");
    #endif
}

void TempMonitorGUI::saveSettings()
{
    QSettings settings("TemperatureMonitor", "TempMonitorGUI");
    
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());
    
    settings.setValue("settings/period", periodCombo->currentIndex());
    settings.setValue("settings/updateInterval", updateIntervalSpin->value());
    settings.setValue("settings/autoUpdate", autoUpdateCheck->isChecked());
    settings.setValue("settings/serverUrl", serverUrl);
}

void TempMonitorGUI::loadSettings()
{
    QSettings settings("TemperatureMonitor", "TempMonitorGUI");
    
    restoreGeometry(settings.value("window/geometry").toByteArray());
    restoreState(settings.value("window/state").toByteArray());
    
    periodCombo->setCurrentIndex(settings.value("settings/period", 1).toInt());
    updateIntervalSpin->setValue(settings.value("settings/updateInterval", 10).toInt());
    autoUpdateCheck->setChecked(settings.value("settings/autoUpdate", false).toBool());
    serverUrl = settings.value("settings/serverUrl", "http://localhost:8080/api/range").toString();
    
    if (autoUpdateCheck->isChecked()) {
        updateTimer->start(updateIntervalSpin->value() * 1000);
    }
}

void TempMonitorGUI::closeEvent(QCloseEvent *event)
{
    saveSettings();
    
    if (isRealtimeMode) {
        onRealtimeToggled(false);
    }
    
    if (updateTimer->isActive()) {
        updateTimer->stop();
    }
    
    event->accept();
}

void TempMonitorGUI::addDataPoint(const QDateTime& timestamp, double temperature)
{
    timeData.append(timestamp.toMSecsSinceEpoch() / 1000.0);
    tempData.append(temperature);
    
    // Ограничиваем размер данных
    const int MAX_POINTS = 1000;
    if (timeData.size() > MAX_POINTS) {
        timeData.removeFirst();
        tempData.removeFirst();
    }
    
    // Обновляем кривую
    curve->setSamples(timeData, tempData);
    plot->replot();
}