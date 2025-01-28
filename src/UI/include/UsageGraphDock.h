#pragma once

#include <QDockWidget>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <vector>
#include "Core/include/ISimulationStatus.h"

class UsageGraphDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit UsageGraphDock(ISimulationStatus *status, QWidget *parent = nullptr);
    ~UsageGraphDock();

private:
    void onTimer();

private:
    ISimulationStatus *m_status{nullptr};

    QTimer m_timer;

    QChartView *m_chartView{nullptr};
    QChart *m_chart{nullptr};
    std::vector<QLineSeries *> m_series;

    static constexpr double AXIS_X_RANGE = 100e3; // 10e3 seconds
    static constexpr double AXIS_X_MIN = 1e3;     // 1e3 seconds
    static const int INTERVAL_MS = 10;
    static const int MAX_POINTS = 60 * 1000 / INTERVAL_MS; // 1 minute

    void createUI();
};