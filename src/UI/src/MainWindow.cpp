#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QtCharts/QChart>

#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_cpuSeries(new QLineSeries(this)), m_ramSeries(new QLineSeries(this)), m_diskSeries(new QLineSeries(this)), m_bandwidthSeries(new QLineSeries(this)), m_axisX(new QValueAxis(this)), m_axisY(new QValueAxis(this)), m_startTime(0.0)
{
    ui->setupUi(this);

    // Create chart
    auto *chart = new QChart();
    chart->addSeries(m_cpuSeries);
    chart->addSeries(m_ramSeries);
    chart->addSeries(m_diskSeries);
    chart->addSeries(m_bandwidthSeries);

    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);

    m_cpuSeries->attachAxis(m_axisX);
    m_cpuSeries->attachAxis(m_axisY);

    m_ramSeries->attachAxis(m_axisX);
    m_ramSeries->attachAxis(m_axisY);

    m_diskSeries->attachAxis(m_axisX);
    m_diskSeries->attachAxis(m_axisY);

    m_bandwidthSeries->attachAxis(m_axisX);
    m_bandwidthSeries->attachAxis(m_axisY);

    m_cpuSeries->setName("CPU Usage");
    m_ramSeries->setName("RAM Usage");
    m_diskSeries->setName("Disk Usage");
    m_bandwidthSeries->setName("Bandwidth Usage");

    m_axisX->setTitleText("Time (s)");
    m_axisY->setTitleText("Usage (%)");

    m_axisX->setRange(0, 500);
    m_axisY->setRange(0, 100);

    m_axisX->setTickCount(10);
    m_axisY->setTickCount(10);

    chart->setTitle("Real-Time Usage");
    ui->chartView->setChart(chart);

    // Update timer
    connect(&m_updateTimer, &QTimer::timeout,
            this, &MainWindow::onUpdateTimerTimeout);
    m_updateTimer.start(10); // 1-second updates
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnLoadTrace_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Open Trace File", "", "*.txt *.csv");
    if (file.isEmpty())
        return;

    m_controller.stopSimulation(); // in case
    m_controller.initialize(file);
    QMessageBox::information(this, "Load Trace", "Trace loaded: " + file);
}

void MainWindow::on_btnStart_clicked()
{
    m_cpuSeries->clear();
    m_ramSeries->clear();
    m_diskSeries->clear();
    m_bandwidthSeries->clear();
    m_startTime = 0.0;
    m_controller.startSimulation();
}

void MainWindow::on_btnStop_clicked()
{
    m_controller.stopSimulation();
}

void MainWindow::on_bundleSizeSpinBox_valueChanged(int arg1)
{
    m_controller.setBundleSize(arg1);
}

void MainWindow::onUpdateTimerTimeout()
{
    auto [t, perResources] = m_controller.getUsageSnapshot();
    m_cpuSeries->append(t, perResources.cpu);
    m_ramSeries->append(t, perResources.ram);
    m_diskSeries->append(t, perResources.disk);
    m_bandwidthSeries->append(t, perResources.bandwidth);

    // Adjust axis if needed
    if (t > m_axisX->max())
    {
        m_axisX->setRange(0, t + 100);
    }
    double maxUsage = std::max(std::max(std::max(perResources.cpu, perResources.ram), perResources.disk), perResources.bandwidth);
    if (maxUsage > m_axisY->max())
    {
        m_axisY->setRange(m_axisY->min(), maxUsage + 10);
    }
}