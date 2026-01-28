#ifndef TEMP_MONITOR_GUI_H
#define TEMP_MONITOR_GUI_H

// Макросы для кроссплатформенности
#ifdef _WIN32
    #define TEMP_EXPORT __declspec(dllexport)
    #define TEMP_IMPORT __declspec(dllimport)
#else
    #define TEMP_EXPORT __attribute__((visibility("default")))
    #define TEMP_IMPORT
#endif

// Макрос для универсальных строк
#ifdef UNICODE
    #define TEMP_TEXT(text) L##text
#else
    #define TEMP_TEXT(text) text
#endif

#include <QMainWindow>
#include <QWidget>
#include <QTimer>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QStatusBar>
#include <QProgressBar>
#include <QThread>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <memory>

// Forward declaration для Qwt
class QwtPlot;
class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;

QT_BEGIN_NAMESPACE
namespace Ui { class TempMonitorGUI; }
QT_END_NAMESPACE

class TemperatureDataFetcher;

class TEMP_EXPORT TempMonitorGUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit TempMonitorGUI(QWidget *parent = nullptr);
    ~TempMonitorGUI();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onFetchData();
    void onRealtimeToggled(bool checked);
    void onPeriodChanged(int index);
    void onUpdateIntervalChanged(int value);
    void onDataFetched(const QJsonArray& data, bool isRealtime);
    void onFetchError(const QString& error);
    void updateCurrentTime();
    void onSaveData();
    void onLoadData();
    void onClearData();
    void onShowStats();

private:
    void setupUI();
    void setupChart();
    void setupConnections();
    void updateChart(const QJsonArray& data);
    void updateStats(const QJsonArray& data);
    void saveSettings();
    void loadSettings();
    void addDataPoint(const QDateTime& timestamp, double temperature);

    // Кроссплатформенные вспомогательные функции
    QString getAppDataPath() const;
    QString formatDateTime(const QDateTime& dt) const;

    // GUI элементы
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QHBoxLayout *controlLayout;
    QHBoxLayout *timeLayout;
    QHBoxLayout *chartLayout;
    QHBoxLayout *statsLayout;

    QGroupBox *controlGroup;
    QGroupBox *chartGroup;
    QGroupBox *statsGroup;

    QLabel *currentTimeLabel;
    QLabel *currentTempLabel;
    QLabel *avgTempLabel;
    QLabel *minTempLabel;
    QLabel *maxTempLabel;

    QComboBox *periodCombo;
    QSpinBox *updateIntervalSpin;
    QDateTimeEdit *fromDateTime;
    QDateTimeEdit *toDateTime;

    QPushButton *fetchButton;
    QPushButton *realtimeButton;
    QPushButton *saveButton;
    QPushButton *loadButton;
    QPushButton *clearButton;
    QPushButton *statsButton;

    QCheckBox *autoUpdateCheck;

    QProgressBar *progressBar;
    QStatusBar *statusBar;

    // График Qwt
    QwtPlot *plot;
    QwtPlotCurve *curve;
    QwtPlotGrid *grid;
    QwtPlotMarker *currentMarker;

    // Данные
    std::unique_ptr<TemperatureDataFetcher> fetcher;
    QTimer *updateTimer;
    QTimer *clockTimer;
    QNetworkAccessManager *networkManager;

    QVector<double> timeData;
    QVector<double> tempData;

    bool isRealtimeMode;
    int currentPeriod;
    QString serverUrl;

    // Статистика
    struct Statistics {
        double current = 0.0;
        double average = 0.0;
        double minimum = 0.0;
        double maximum = 0.0;
        int count = 0;
    } stats;
};

class TemperatureDataFetcher : public QObject
{
    Q_OBJECT

public:
    explicit TemperatureDataFetcher(QObject *parent = nullptr);

public slots:
    void fetchData(const QString& url, bool realtime = false);

signals:
    void dataReady(const QJsonArray& data, bool isRealtime);
    void error(const QString& message);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager manager;
};

#endif // TEMP_MONITOR_GUI_H