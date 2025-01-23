#pragma once

#include <QDockWidget>
#include <QComboBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QPushButton>
#include "strategies/IPlacementStrategy.h"

/**
 * A dock that allows user to configure strategies,
 * pick a strategy from a combo, etc.
 * You can expand it with your actual logic
 */
class StrategyConfigDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit StrategyConfigDock(QWidget *parent = nullptr);
    ~StrategyConfigDock();

private slots:
    void onStrategyChanged(int index);
    void onApplyClicked();

private:
    QWidget *m_container;
    QFormLayout *m_formLayout;

    QComboBox *m_strategyCombo;
    QPushButton *m_applyBtn;

    // The current strategy pointer
    IPlacementStrategy *m_currentStrategy;
    // The widget returned by that strategy
    QWidget *m_currentStrategyWidget;

    void createUI();
    void loadStrategyList();
    void clearCurrentStrategy();
};