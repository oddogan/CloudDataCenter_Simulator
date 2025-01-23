#pragma once

#include <QDockWidget>
#include <QComboBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QPushButton>
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

private slots:
    void onStrategyChanged(int index);
    void onStrategyApplyClicked();
    void onStrategyResetClicked();
    void onBundleSizeApplyClicked();
    void onBundleSizeResetClicked();

private:
    ISimulationConfiguration *m_simulator;
    QWidget *m_container;
    QFormLayout *m_formLayout;

    QSpinBox *m_bundleSizeSpin;
    QPushButton *m_bundleSizeApplyButton;
    QPushButton *m_bundleSizeResetButton;

    QComboBox *m_strategyCombo;
    QPushButton *m_strategyApplyBtn;
    QPushButton *m_strategyResetBtn;
    IPlacementStrategy *m_currentStrategy;
    QWidget *m_currentStrategyWidget;

    void createUI();
    void loadStrategyList();
    void clearCurrentStrategy();
};