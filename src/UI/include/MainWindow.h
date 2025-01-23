#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include "Core/include/TraceReader.h"
#include "Core/include/SimulationEngine.h"

// Forward declares for our custom docks
class StrategyConfigDock;
class LoggingDock;

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
    explicit MainWindow(TraceReader &traceReader, SimulationEngine &simulationEngine, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionOpenTrace_triggered();
    void on_actionStartSimulation_triggered();
    void on_actionStopSimulation_triggered();
    void on_actionQuit_triggered();
    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;

    StrategyConfigDock *m_strategyDock;
    LoggingDock *m_loggingDock;
    QWidget *m_usageChart;

    TraceReader &m_traceReader;
    SimulationEngine &m_simulationEngine;

    void setupDocks();
    void setupCentralWidget();
    void setupViewMenu();
};