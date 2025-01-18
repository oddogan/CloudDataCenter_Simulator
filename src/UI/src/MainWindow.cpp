#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QtCharts/QChart>

#include <algorithm>

#include "strategies/StrategyFactory.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_cpuSeries(new QLineSeries(this)), m_ramSeries(new QLineSeries(this)), m_diskSeries(new QLineSeries(this)), m_bandwidthSeries(new QLineSeries(this)), m_axisX(new QValueAxis(this)), m_axisY(new QValueAxis(this)), m_startTime(0.0), m_currentStrategy(nullptr), m_currentConfigWidget(nullptr)
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

    // Fill strategy combo
    auto names = StrategyFactory::availableStrategies();
    for (auto &n : names)
    {
        ui->strategyComboBox->addItem(n);
    }
    // connect signals
    connect(ui->strategyComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onStrategyChanged);

    connect(ui->applyStrategyButton,
            &QPushButton::clicked,
            this,
            &MainWindow::onApplyStrategyButtonClicked);

    // select first strategy
    ui->strategyComboBox->setCurrentIndex(0);
    onStrategyChanged(0);
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

void MainWindow::onStrategyChanged(int index)
{
    QString name = ui->strategyComboBox->itemText(index);

    // Make a new strategy
    IConfigurableStrategy *newStrategy = StrategyFactory::create(name);

    // Make its config widget
    QWidget *w = newStrategy->createConfigWidget(this);

    // Remove old widget from layout if any
    if (m_currentConfigWidget)
    {
        // find layout in strategyConfigContainer
        auto layout = ui->strategyConfigContainer->layout();
        if (layout)
        {
            layout->removeWidget(m_currentConfigWidget);
        }
        m_currentConfigWidget->deleteLater();
        m_currentConfigWidget = nullptr;
    }

    // If no layout yet, create one
    if (!ui->strategyConfigContainer->layout())
    {
        auto vbox = new QVBoxLayout(ui->strategyConfigContainer);
        ui->strategyConfigContainer->setLayout(vbox);
    }
    ui->strategyConfigContainer->layout()->addWidget(w);
    m_currentConfigWidget = w;

    // If we had an old strategy not applied to DataCenter, delete it
    if (m_currentStrategy)
    {
        delete m_currentStrategy;
        m_currentStrategy = nullptr;
    }

    m_currentStrategy = newStrategy;
}

// When user clicks "Apply", we finalize and tell DataCenter
void MainWindow::onApplyStrategyButtonClicked()
{
    if (!m_currentStrategy)
        return;
    // let the strategy read from its widget
    m_currentStrategy->applyConfigFromUI();

    // Now pass it to data center. We assume dataCenter takes ownership
    m_controller.setStrategy(m_currentStrategy);

    // So we don't double delete
    m_currentStrategy = nullptr;

    qDebug() << "[MainWindow] Strategy applied to DataCenter.";
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