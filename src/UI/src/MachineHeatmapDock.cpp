#include "MachineHeatmapDock.h"

#include <QPainter>
#include <QToolTip>
#include <QMouseEvent>
#include <QDebug>

HeatmapWidget::HeatmapWidget(ISimulationStatus *status, QWidget *parent)
    : QWidget(parent), m_status(status)
{
    setMouseTracking(true);
    connect(&m_timer, &QTimer::timeout, this, &HeatmapWidget::updateUsage);
    m_timer.start(100);
    updateUsage();
}

HeatmapWidget::~HeatmapWidget()
{
}

void HeatmapWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

void HeatmapWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    int n = static_cast<int>(m_machineInfos.size());
    if (n == 0)
    {
        painter.drawText(rect(), Qt::AlignCenter, "No data");
        return;
    }

    // 1) Decide how many columns, rows
    int columns = static_cast<int>(std::ceil(std::sqrt(n)));
    int rows = (n + columns - 1) / columns;

    // 2) Decide cell size
    double w = width();
    double h = height();

    double cellSize = std::max(MIN_CELL, std::min(MAX_CELL, static_cast<int>(std::min(w / columns, h / rows))));

    // 3) Draw cells
    for (int i = 0; i < n; ++i)
    {
        int col = i % columns;
        int row = i / columns;

        int x = col * cellSize;
        int y = row * cellSize;

        auto &info = m_machineInfos[i];
        double cpuUsage = info.total.cpu > 0 ? (double)info.used.cpu / info.total.cpu : 0.0;
        double ramUsage = info.total.ram > 0 ? (double)info.used.ram / info.total.ram : 0.0;
        double diskUsage = info.total.disk > 0 ? (double)info.used.disk / info.total.disk : 0.0;
        double fpgaUsage = info.total.fpga > 0 ? (double)info.used.fpga / info.total.fpga : 0.0;

        QColor cpuColor = usageColor(cpuUsage);
        QColor ramColor = usageColor(ramUsage);
        QColor diskColor = usageColor(diskUsage);
        QColor fpgaColor = usageColor(fpgaUsage);

        // Add a black border to the cells, fill them according to the usages
        painter.setPen(Qt::black);
        painter.setBrush(cpuColor);
        painter.drawRect(x, y, cellSize / 2, cellSize / 2);
        painter.setBrush(ramColor);
        painter.drawRect(x + cellSize / 2, y, cellSize / 2, cellSize / 2);
        painter.setBrush(diskColor);
        painter.drawRect(x, y + cellSize / 2, cellSize / 2, cellSize / 2);
        painter.setBrush(fpgaColor);
        painter.drawRect(x + cellSize / 2, y + cellSize / 2, cellSize / 2, cellSize / 2);
    }
}

void HeatmapWidget::mouseMoveEvent(QMouseEvent *event)
{
    int n = static_cast<int>(m_machineInfos.size());
    if (n == 0)
        return;

    int columns = static_cast<int>(std::ceil(std::sqrt(n)));
    int rows = (n + columns - 1) / columns;

    double w = width();
    double h = height();

    double cellSize = std::max(MIN_CELL, std::min(MAX_CELL, static_cast<int>(std::min(w / columns, h / rows))));

    int col = event->position().x() / cellSize;
    int row = event->position().y() / cellSize;

    int i = row * columns + col;
    if (i < 0 || i >= n)
        return;

    auto &info = m_machineInfos[i];
    double cpuUsage = info.total.cpu > 0 ? info.used.cpu / info.total.cpu * 100.0 : 0.0;
    double ramUsage = info.total.ram > 0 ? info.used.ram / info.total.ram * 100.0 : 0.0;
    double diskUsage = info.total.disk > 0 ? info.used.disk / info.total.disk * 100.0 : 0.0;
    double fpgaUsage = info.total.fpga > 0 ? info.used.fpga / info.total.fpga * 100.0 : 0.0;

    QToolTip::showText(event->globalPosition().toPoint(), QString("PM #%1\nCPU: %2%\nRAM: %3%\nDisk: %4%\nFPGA: %5%")
                                                              .arg(info.machineId)
                                                              .arg(cpuUsage)
                                                              .arg(ramUsage)
                                                              .arg(diskUsage)
                                                              .arg(fpgaUsage));
}

void HeatmapWidget::updateUsage()
{
    if (!m_status)
        return;

    m_machineInfos = m_status->getMachineUsageInfo();
    update();
}

QColor HeatmapWidget::usageColor(double usageRatio) const
{
    if (usageRatio < 0.0)
        usageRatio = 0.0;
    if (usageRatio > 1.0)
        usageRatio = 1.0;

    int r = 255 * usageRatio;
    int g = 255 * (1.0 - usageRatio);

    return QColor(r, g, 0);
}

MachineHeatmapDock::MachineHeatmapDock(ISimulationStatus *status, QWidget *parent)
    : QDockWidget(parent)
{
    setWindowTitle("Machine Heatmap");
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    m_heatmap = new HeatmapWidget(status, this);
    setWidget(m_heatmap);
}

MachineHeatmapDock::~MachineHeatmapDock()
{
}