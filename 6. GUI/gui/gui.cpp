#include "gui.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTimeZone>
#include <QNetworkReply>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), currentPeriodSeconds(60) {
    setupUi();
    networkManager = new QNetworkAccessManager(this);
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::updateCurrentData);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::updateHistoryData);
    refreshTimer->start(1000);
    updateCurrentData();
    updateHistoryData();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    QHBoxLayout *topLayout = new QHBoxLayout();
    QVBoxLayout *tempLayout = new QVBoxLayout();
    currentTempLabel = new QLabel("Current: -- °C");
    QFont fontCurr = currentTempLabel->font();
    fontCurr.setPointSize(20);
    fontCurr.setBold(true);
    currentTempLabel->setFont(fontCurr);
    averageTempLabel = new QLabel("Avg (Period): -- °C");
    averageTempLabel->setStyleSheet("color: #666;");
    tempLayout->addWidget(currentTempLabel);
    tempLayout->addWidget(averageTempLabel);
    lastUpdateLabel = new QLabel("Last update: --");
    topLayout->addLayout(tempLayout);
    topLayout->addStretch();
    topLayout->addWidget(lastUpdateLabel);
    mainLayout->addLayout(topLayout);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnMinute = new QPushButton("Last Minute");
    QPushButton *btnHour = new QPushButton("Last Hour");
    QPushButton *btnDay = new QPushButton("Last Day");
    connect(btnMinute, &QPushButton::clicked, [this](){ onPeriodChanged(60); });
    connect(btnHour, &QPushButton::clicked, [this](){ onPeriodChanged(3600); });
    connect(btnDay, &QPushButton::clicked, [this](){ onPeriodChanged(86400); });
    btnLayout->addWidget(btnMinute);
    btnLayout->addWidget(btnHour);
    btnLayout->addWidget(btnDay);
    mainLayout->addLayout(btnLayout);
    
    series = new QLineSeries();
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->hide();
    axisX = new QDateTimeAxis();
    axisX->setFormat("HH:mm:ss");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    axisY = new QValueAxis();
    axisY->setRange(10, 40);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    mainLayout->addWidget(chartView);
    
    setCentralWidget(centralWidget);
    resize(900, 600);
    setWindowTitle("Weather Station GUI");
}

void MainWindow::onPeriodChanged(int seconds) {
    currentPeriodSeconds = seconds;
    if (seconds > 3600) axisX->setFormat("HH:mm"); 
    else axisX->setFormat("HH:mm:ss");
    updateHistoryData();
}

void MainWindow::updateCurrentData() {
    QNetworkRequest request((QUrl("http://localhost:8080/api/current")));
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onCurrentDataReceived(reply); });
}

void MainWindow::updateHistoryData() {
    QString url = QString("http://localhost:8080/api/history?seconds=%1").arg(currentPeriodSeconds);
    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onHistoryDataReceived(reply); });
}

void MainWindow::onCurrentDataReceived(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        double temp = obj["temp"].toDouble();
        QString time = obj["time"].toString();
        currentTempLabel->setText(QString("Current: %1 °C").arg(temp, 0, 'f', 2));
        lastUpdateLabel->setText("Last update: " + time);
    }
    reply->deleteLater();
}

void MainWindow::onHistoryDataReceived(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray arr = doc.array();
        QList<QPointF> points;
        double minTemp = 100, maxTemp = -100, sumTemp = 0;
        for (const auto &val : arr) {
            QJsonObject obj = val.toObject();
            double temp = obj["temp"].toDouble();
            QString timeStr = obj["time"].toString();
            QDateTime dt = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");
            if (!dt.isValid()) dt = QDateTime::fromString(timeStr, Qt::ISODate);
            if (dt.isValid()) {
                #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                    dt.setTimeSpec(Qt::UTC);
                #else
                    dt.setTimeZone(QTimeZone::UTC);
                #endif
                QDateTime localDt = dt.toLocalTime();
                points.append(QPointF(localDt.toMSecsSinceEpoch(), temp));
                sumTemp += temp;
                if (temp < minTemp) minTemp = temp;
                if (temp > maxTemp) maxTemp = temp;
            }
        }
        if (!points.isEmpty()) {
            double avg = sumTemp / points.size();
            averageTempLabel->setText(QString("Avg (Period): %1 °C").arg(avg, 0, 'f', 2));
            std::sort(points.begin(), points.end(), [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });
            series->replace(points);
            axisY->setRange(minTemp - 2, maxTemp + 2);
            QDateTime first = QDateTime::fromMSecsSinceEpoch(points.first().x());
            QDateTime last = QDateTime::fromMSecsSinceEpoch(points.last().x());
            axisX->setRange(first, last);
        } else {
            averageTempLabel->setText("Avg (Period): -- °C");
            series->clear();
        }
    }
    reply->deleteLater();
}