#pragma once
#include <QMainWindow>
#include <QtCharts>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void updateCurrentData();
    void updateHistoryData();
    void onPeriodChanged(int seconds);
    void onCurrentDataReceived(QNetworkReply *reply);
    void onHistoryDataReceived(QNetworkReply *reply);
private:
    void setupUi();
    QLabel *currentTempLabel, *averageTempLabel, *lastUpdateLabel;
    QChartView *chartView;
    QLineSeries *series;
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
    QNetworkAccessManager *networkManager;
    QTimer *refreshTimer;
    int currentPeriodSeconds;
};