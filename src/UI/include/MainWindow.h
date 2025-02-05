#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QSettings>
#include <QCloseEvent>
#include "Core/include/TraceReader.h"
#include "Core/include/SimulationEngine.h"

class ConfigurationDock;
class LoggingDock;
class StatusDock;
class MachineDock;
class MachineHeatmapDock;
class UsageGraphDock;

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

private:
    void on_actionOpenTrace_triggered();
    void on_actionStartSimulation_triggered();
    void on_actionStopSimulation_triggered();
    void on_actionQuit_triggered();
    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui{nullptr};

    ConfigurationDock *m_configDock{nullptr};
    LoggingDock *m_loggingDock{nullptr};
    StatusDock *m_statusDock{nullptr};
    MachineDock *m_machineDock{nullptr};
    MachineHeatmapDock *m_heatmapDock{nullptr};
    UsageGraphDock *m_usageGraphDock{nullptr};

    TraceReader &m_traceReader;
    SimulationEngine &m_simulationEngine;

    void setupDocks();
    void setupCentralWidget();
    void setupViewMenu();
};