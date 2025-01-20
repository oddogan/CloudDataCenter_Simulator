#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <QDialog>
#include "logging/LogManager.h"

class LoggingPanel : public QDialog
{
    Q_OBJECT

public:
    explicit LoggingPanel(QWidget *parent = nullptr);
    ~LoggingPanel();

private slots:
    void onApplyClicked();
    void onBrowseClicked();

private:
    QLineEdit *m_filePathEdit;
    QCheckBox *m_placementCheck;
    QCheckBox *m_vmArrivalCheck;
    QCheckBox *m_vmDepartureCheck;
    QCheckBox *m_migrationCheck;
    QCheckBox *m_vmUtilUpdateCheck;
    QCheckBox *m_traceInfoCheck;
    QCheckBox *m_debugCheck;

    QPushButton *m_applyBtn;
    QPushButton *m_browseBtn;

    void loadFromManager();
    void saveToManager();
};