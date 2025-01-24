#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QMessageBox>
#include <QDockWidget>
#include <QMenu>
#include <QFileDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

#include "ConfigurationDock.h"
#include "LoggingDock.h"
#include "StatusDock.h"
#include "MachineDock.h"
#include "MachineHeatmapDock.h"
#include "UsageGraphDock.h"

MainWindow::MainWindow(TraceReader &traceReader, SimulationEngine &simulationEngine, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_traceReader(traceReader), m_simulationEngine(simulationEngine)
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

    resize(1920, 1080);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupDocks()
{
    m_configDock = new ConfigurationDock(&m_simulationEngine, this);
    m_configDock->setObjectName("ConfigDock");
    addDockWidget(Qt::LeftDockWidgetArea, m_configDock);
    m_configDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_loggingDock = new LoggingDock(this);
    m_loggingDock->setObjectName("LoggingDock");
    addDockWidget(Qt::LeftDockWidgetArea, m_loggingDock);
    m_loggingDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_statusDock = new StatusDock(&m_simulationEngine, this);
    m_statusDock->setObjectName("StatusDock");
    addDockWidget(Qt::BottomDockWidgetArea, m_statusDock);
    m_statusDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    // m_machineDock = new MachineDock(&m_simulationEngine, this);
    // m_machineDock->setObjectName("MachineDock");
    // addDockWidget(Qt::BottomDockWidgetArea, m_machineDock);
    // m_machineDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    m_heatmapDock = new MachineHeatmapDock(&m_simulationEngine, this);
    m_heatmapDock->setObjectName("MachineHeatmapDock");
    addDockWidget(Qt::RightDockWidgetArea, m_heatmapDock);
    m_heatmapDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_heatmapDock->setMinimumWidth(530);
}

void MainWindow::setupCentralWidget()
{
    m_usageGraphDock = new UsageGraphDock(&m_simulationEngine, this);
    setCentralWidget(m_usageGraphDock);
}

void MainWindow::setupViewMenu()
{
    QMenu *vMenu = ui->menuView;
    if (!vMenu)
        return;

    QAction *stratDockToggle = m_configDock->toggleViewAction();
    stratDockToggle->setText("Configuration Dock");
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