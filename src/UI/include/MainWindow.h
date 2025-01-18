#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include "SimulatorController.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnLoadTrace_clicked();
    void on_btnStart_clicked();
    void on_btnStop_clicked();
    void on_bundleSizeSpinBox_valueChanged(int arg1);

    // For real-time updates
    void onUpdateTimerTimeout();

private:
    Ui::MainWindow *ui;
    SimulatorController m_controller;

    QLineSeries *m_cpuSeries;
    QLineSeries *m_ramSeries;
    QLineSeries *m_diskSeries;
    QLineSeries *m_bandwidthSeries;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;

    QTimer m_updateTimer;
    double m_startTime;
};