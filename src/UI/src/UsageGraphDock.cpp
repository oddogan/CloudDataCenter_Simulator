#include "UsageGraphDock.h"
#include <QtCharts/QValueAxis>

UsageGraphDock::UsageGraphDock(ISimulationStatus *status, QWidget *parent)
    : QDockWidget(parent), m_status(status)
{
    setWindowTitle("Usage Graph");
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    createUI();

    connect(&m_timer, &QTimer::timeout, this, &UsageGraphDock::onTimer);
    m_timer.start(INTERVAL_MS);
}

UsageGraphDock::~UsageGraphDock()
{
    for (auto series : m_series)
    {
        delete series;
    }
}

void UsageGraphDock::createUI()
{
    m_chart = new QChart();
    m_chart->setTitle("Resource Utilizations");
    m_chart->legend()->setVisible(true);

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    setWidget(m_chartView);

    // Create series
    auto seriesCPU = new QLineSeries();
    seriesCPU->setName("CPU");
    m_chart->addSeries(seriesCPU);
    m_series.push_back(seriesCPU);
    auto seriesRAM = new QLineSeries();
    seriesRAM->setName("RAM");
    m_chart->addSeries(seriesRAM);
    m_series.push_back(seriesRAM);
    auto seriesDisk = new QLineSeries();
    seriesDisk->setName("Disk");
    m_chart->addSeries(seriesDisk);
    m_series.push_back(seriesDisk);
    auto seriesBandwidth = new QLineSeries();
    seriesBandwidth->setName("Bandwidth");
    m_chart->addSeries(seriesBandwidth);
    m_series.push_back(seriesBandwidth);
    auto seriesFPGA = new QLineSeries();
    seriesFPGA->setName("FPGA");
    m_chart->addSeries(seriesFPGA);
    m_series.push_back(seriesFPGA);

    // Create axes
    auto axisX = new QValueAxis();
    axisX->setRange(0, AXIS_X_MIN);
    axisX->setLabelFormat("%d");
    axisX->setTitleText("Time (s)");
    m_chart->addAxis(axisX, Qt::AlignBottom);

    auto axisY = new QValueAxis();
    axisY->setRange(0, 110);
    axisY->setTickCount(12);
    axisY->setLabelFormat("%d");
    axisY->setTitleText("Usage (%)");
    m_chart->addAxis(axisY, Qt::AlignLeft);

    // Attach series to axes
    for (auto series : m_series)
    {
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }
}

void UsageGraphDock::onTimer()
{
    if (!m_status)
        return;

    auto utilizations = m_status->getResourceUtilizations();
    for (size_t i = 0; i < m_series.size(); i++)
    {
        auto series = m_series[i];
        double value = 0.0;
        switch (i)
        {
        case 0:
            value = utilizations.utilizations.cpu;
            break;
        case 1:
            value = utilizations.utilizations.ram;
            break;
        case 2:
            value = utilizations.utilizations.disk;
            break;
        case 3:
            value = utilizations.utilizations.bandwidth;
            break;
        case 4:
            value = utilizations.utilizations.fpga;
            break;
        default:
            break;
        }

        series->append(utilizations.time, value);
        if (series->count() > MAX_POINTS)
        {
            series->removePoints(0, series->count() - MAX_POINTS);
        }

        // Set minimum and maximum sizes for X axis
        auto axisX = m_chart->axes(Qt::Horizontal).at(0);

        axisX->setRange(std::max(0.0, utilizations.time - AXIS_X_RANGE), std::max(utilizations.time + AXIS_X_MIN, AXIS_X_MIN));
    }
}