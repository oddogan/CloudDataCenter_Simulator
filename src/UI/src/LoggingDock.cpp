#include "LoggingDock.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QDebug>

LoggingDock::LoggingDock(QWidget *parent)
    : QDockWidget(parent), m_container(nullptr)
{
    setWindowTitle("Logging");
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    createUI();
    loadFromManager();
}

LoggingDock::~LoggingDock()
{
}

void LoggingDock::createUI()
{

    m_container = new QWidget(this);
    auto layout = new QFormLayout(m_container);

    m_logToConsoleCheckBox = new QCheckBox("Log to Console", m_container);
    layout->addRow(m_logToConsoleCheckBox);
    connect(m_logToConsoleCheckBox, &QCheckBox::checkStateChanged, this, &LoggingDock::onLogToConsoleStateChanged);

    // Log file row
    m_filePathEdit = new QLineEdit(m_container);
    m_browseBtn = new QPushButton("Browse", m_container);

    QHBoxLayout *fileRow = new QHBoxLayout();
    fileRow->addWidget(m_filePathEdit);
    fileRow->addWidget(m_browseBtn);

    layout->addRow("Log File:", fileRow);

    // build checkboxes from LogManager's category list
    auto cats = LogManager::instance().getAllCategories();
    for (auto &info : cats)
    {
        CategoryWidget cw;
        cw.cat = info.cat;
        cw.checkBox = new QCheckBox(QString::fromStdString(info.displayName), m_container);
        m_categoryWidgets.push_back(cw);
        layout->addRow(cw.checkBox);

        connect(cw.checkBox, &QCheckBox::checkStateChanged, this, &LoggingDock::onCheckBoxStateChanged);
    }

    m_applyBtn = new QPushButton("Apply", m_container);
    layout->addRow(m_applyBtn);

    connect(m_applyBtn, &QPushButton::clicked, this, &LoggingDock::onApplyClicked);
    connect(m_browseBtn, &QPushButton::clicked, this, &LoggingDock::onBrowseClicked);

    m_container->setLayout(layout);
    setWidget(m_container);
}

void LoggingDock::loadFromManager()
{
    auto &mgr = LogManager::instance();

    m_logToConsoleCheckBox->setChecked(mgr.getLogToConsole());

    m_filePathEdit->setText(QString::fromStdString(mgr.getLogFile()));

    // For each category
    for (auto &cw : m_categoryWidgets)
    {
        bool en = mgr.isCategoryEnabled(cw.cat);
        cw.checkBox->setChecked(en);
    }
}

void LoggingDock::saveToManager()
{
    auto &mgr = LogManager::instance();
    mgr.setLogFile(m_filePathEdit->text().toStdString());

    for (auto &cw : m_categoryWidgets)
    {
        mgr.setCategoryEnabled(cw.cat, cw.checkBox->isChecked());
    }
}

void LoggingDock::onApplyClicked()
{
    saveToManager();
    qDebug() << "[LoggingDock] Applied logging settings.";
}

void LoggingDock::onBrowseClicked()
{
    QString path = QFileDialog::getSaveFileName(this, "Select Log File");
    if (!path.isEmpty())
    {
        m_filePathEdit->setText(path);
    }
}

void LoggingDock::onCheckBoxStateChanged(int state)
{
    QCheckBox *cb = qobject_cast<QCheckBox *>(sender());
    if (!cb)
    {
        return;
    }

    // Find the category
    auto it = std::find_if(m_categoryWidgets.begin(), m_categoryWidgets.end(), [cb](const CategoryWidget &cw)
                           { return cw.checkBox == cb; });

    if (it == m_categoryWidgets.end())
    {
        return;
    }

    LogCategory cat = it->cat;
    bool enabled = state == Qt::Checked;
    LogManager::instance().setCategoryEnabled(cat, enabled);
    qDebug() << "[LoggingDock] Category " << LogManager::instance().getCategoryDisplayName(cat) << " set to" << enabled;
}

void LoggingDock::onLogToConsoleStateChanged(int state)
{
    bool enabled = state == Qt::Checked;
    LogManager::instance().setLogToConsole(enabled);
    qDebug() << "[LoggingDock] Log to console set to" << enabled;
}
