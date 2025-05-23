#pragma once

#include <QDockWidget>
#include <QComboBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include "strategies/IPlacementStrategy.h"
#include "ISimulationConfiguration.h"

/**
 * A dock that allows user to configure strategies,
 * pick a strategy from a combo, etc.
 * You can expand it with your actual logic
 */
class ConfigurationDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit ConfigurationDock(ISimulationConfiguration *simulator, QWidget *parent = nullptr);
    ~ConfigurationDock();

private:
    void onStrategyChanged(int index);
    void onStrategyApplyClicked();
    void onStrategyResetClicked();
    void onOutputFileBrowseClicked();
    void onOutputFileApplyClicked();

private:
    ISimulationConfiguration *m_simulator{nullptr};
    QWidget *m_container{nullptr};
    QFormLayout *m_formLayout{nullptr};

    QLineEdit *m_outputFilePathEdit{nullptr};
    QPushButton *m_outputFileBrowseBtn{nullptr};
    QPushButton *m_outputFileApplyBtn{nullptr};

    QComboBox *m_strategyCombo{nullptr};
    QPushButton *m_strategyApplyBtn{nullptr};
    QPushButton *m_strategyResetBtn{nullptr};
    IPlacementStrategy *m_currentStrategy{nullptr};
    QWidget *m_currentStrategyWidget{nullptr};
    QWidget *m_currentStrategyStatusWidget{nullptr};

    void createUI();
    void loadStrategyList();
    void clearCurrentStrategy(bool deleteStrategy);
};