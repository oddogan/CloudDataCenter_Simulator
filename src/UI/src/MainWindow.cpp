#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QMessageBox>
#include <QDockWidget>
#include <QMenu>
#include <QFileDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

#include "StrategyConfigDock.h"
#include "LoggingDock.h"

MainWindow::MainWindow(TraceReader &traceReader, SimulationEngine &simulationEngine, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_strategyDock(nullptr), m_loggingDock(nullptr), m_usageChart(nullptr), m_traceReader(traceReader), m_simulationEngine(simulationEngine)
{
    ui->setupUi(this);

    /*
    connect(ui->actionOpenTrace, &QAction::triggered, this, &MainWindow::on_actionOpenTrace_triggered);
    connect(ui->actionStartSimulation, &QAction::triggered, this, &MainWindow::on_actionStartSimulation_triggered);
    connect(ui->actionStopSimulation, &QAction::triggered, this, &MainWindow::on_actionStopSimulation_triggered);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::on_actionQuit_triggered);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::on_actionAbout_triggered);
    */

    setupDocks();
    setupCentralWidget();
    setupViewMenu();

    resize(1000, 700);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupDocks()
{
    m_strategyDock = new StrategyConfigDock(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_strategyDock);

    m_loggingDock = new LoggingDock(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_loggingDock);

    m_strategyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_loggingDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
}

void MainWindow::setupCentralWidget()
{
    // usage chart
    auto chart = new QChart();
    chart->setTitle("Resource Utilizations");
    auto chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);

    m_usageChart = chartView;
    setCentralWidget(m_usageChart);
}

void MainWindow::setupViewMenu()
{
    QMenu *vMenu = ui->menuView;
    if (!vMenu)
        return;

    QAction *stratDockToggle = m_strategyDock->toggleViewAction();
    stratDockToggle->setText("Strategy Config Dock");
    QAction *logDockToggle = m_loggingDock->toggleViewAction();
    logDockToggle->setText("Logging Dock");

    vMenu->addAction(stratDockToggle);
    vMenu->addAction(logDockToggle);
}

void MainWindow::on_actionOpenTrace_triggered()
{
    QString file = QFileDialog::getOpenFileName(this, "Open Trace", "", "*.txt *.csv");
    if (!file.isEmpty())
    {
        QMessageBox::information(this, "Trace", "Trace loaded: " + file);
        m_traceReader.readTraceFile(file.toStdString());
    }
}

void MainWindow::on_actionStartSimulation_triggered()
{
    m_simulationEngine.start();
    QMessageBox::information(this, "Simulation", "Simulation started.");
}

void MainWindow::on_actionStopSimulation_triggered()
{
    m_simulationEngine.stop();
    QMessageBox::information(this, "Simulation", "Simulation stopped.");
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About",
                       "Real-Time Cloud Data Center Simulator");
}
