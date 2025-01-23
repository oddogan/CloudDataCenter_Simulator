#pragma once

#include <QDockWidget>
#include <QTimer>
#include <QScrollArea>
#include <QWidget>

#include "data/PhysicalMachine.h"
#include "ISimulationStatus.h"

class HeatmapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HeatmapWidget(ISimulationStatus *status, QWidget *parent = nullptr);
    ~HeatmapWidget();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    ISimulationStatus *m_status;
    std::vector<MachineUsageInfo> m_machineInfos;
    QTimer m_timer;

    const int MIN_CELL = 5;
    const int MAX_CELL = 100;

    void updateUsage();
    QColor usageColor(double usageRatio) const;
};

class MachineHeatmapDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit MachineHeatmapDock(ISimulationStatus *status, QWidget *parent = nullptr);
    ~MachineHeatmapDock();

private:
    HeatmapWidget *m_heatmap{nullptr};
};