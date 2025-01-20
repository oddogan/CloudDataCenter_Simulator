#include "LoggingPanel.h"
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDebug>

LoggingPanel::LoggingPanel(QWidget *parent)
    : QDialog(parent)
{
    auto layout = new QFormLayout(this);

    // Log file path
    m_filePathEdit = new QLineEdit(this);
    m_browseBtn = new QPushButton("Browse...", this);

    auto fileLayout = new QHBoxLayout();
    fileLayout->addWidget(m_filePathEdit);
    fileLayout->addWidget(m_browseBtn);

    layout->addRow("Log File:", fileLayout);

    // Checkboxes
    m_placementCheck = new QCheckBox("Placement", this);
    m_vmArrivalCheck = new QCheckBox("VM Arrival", this);
    m_vmDepartureCheck = new QCheckBox("VM Departure", this);
    m_migrationCheck = new QCheckBox("VM Migration", this);
    m_vmUtilUpdateCheck = new QCheckBox("VM Utilization Update", this);
    m_traceInfoCheck = new QCheckBox("Trace Info", this);
    m_debugCheck = new QCheckBox("Debug", this);

    layout->addRow(m_placementCheck);
    layout->addRow(m_vmArrivalCheck);
    layout->addRow(m_vmDepartureCheck);
    layout->addRow(m_migrationCheck);
    layout->addRow(m_vmUtilUpdateCheck);
    layout->addRow(m_traceInfoCheck);
    layout->addRow(m_debugCheck);

    // Apply button
    m_applyBtn = new QPushButton("Apply", this);
    layout->addRow(m_applyBtn);

    setLayout(layout);

    // connect signals
    connect(m_applyBtn, &QPushButton::clicked, this, &LoggingPanel::onApplyClicked);
    connect(m_browseBtn, &QPushButton::clicked, this, &LoggingPanel::onBrowseClicked);

    // load current settings from LogManager
    loadFromManager();
}

LoggingPanel::~LoggingPanel()
{
}

void LoggingPanel::loadFromManager()
{
    LogManager &mgr = LogManager::instance();

    m_filePathEdit->setText(QString::fromStdString(mgr.getLogFile()));

    m_placementCheck->setChecked(mgr.isCategoryEnabled(LogCategory::PLACEMENT));
    m_vmArrivalCheck->setChecked(mgr.isCategoryEnabled(LogCategory::VM_ARRIVAL));
    m_vmDepartureCheck->setChecked(mgr.isCategoryEnabled(LogCategory::VM_DEPARTURE));
    m_migrationCheck->setChecked(mgr.isCategoryEnabled(LogCategory::VM_MIGRATION));
    m_vmUtilUpdateCheck->setChecked(mgr.isCategoryEnabled(LogCategory::VM_UTIL_UPDATE));
    m_traceInfoCheck->setChecked(mgr.isCategoryEnabled(LogCategory::TRACE));
    m_debugCheck->setChecked(mgr.isCategoryEnabled(LogCategory::DEBUG));
}

void LoggingPanel::saveToManager()
{
    LogManager &mgr = LogManager::instance();

    mgr.setLogFile(m_filePathEdit->text().toStdString());

    mgr.setCategoryEnabled(LogCategory::PLACEMENT, m_placementCheck->isChecked());
    mgr.setCategoryEnabled(LogCategory::VM_ARRIVAL, m_vmArrivalCheck->isChecked());
    mgr.setCategoryEnabled(LogCategory::VM_DEPARTURE, m_vmDepartureCheck->isChecked());
    mgr.setCategoryEnabled(LogCategory::VM_MIGRATION, m_migrationCheck->isChecked());
    mgr.setCategoryEnabled(LogCategory::VM_UTIL_UPDATE, m_vmUtilUpdateCheck->isChecked());
    mgr.setCategoryEnabled(LogCategory::TRACE, m_traceInfoCheck->isChecked());
    mgr.setCategoryEnabled(LogCategory::DEBUG, m_debugCheck->isChecked());
}

void LoggingPanel::onApplyClicked()
{
    saveToManager();
    qDebug() << "[LoggingPanel] Settings applied to LogManager.";
}

void LoggingPanel::onBrowseClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Choose Log File");
    if (!filePath.isEmpty())
    {
        m_filePathEdit->setText(filePath);
    }
}