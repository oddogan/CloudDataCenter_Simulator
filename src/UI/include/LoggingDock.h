#pragma once

#include <QDockWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include "logging/LogManager.h"

class LoggingDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit LoggingDock(QWidget *parent = nullptr);
    ~LoggingDock();

private slots:
    void onApplyClicked();
    void onBrowseClicked();
    void onCheckBoxStateChanged(int state);
    void onLogToConsoleStateChanged(int state);

private:
    QWidget *m_container;

    // We'll store checkboxes in a vector, each paired to a LogCategory
    struct CategoryWidget
    {
        LogCategory cat;
        QCheckBox *checkBox;
    };
    std::vector<CategoryWidget> m_categoryWidgets;

    QLineEdit *m_filePathEdit;
    QPushButton *m_browseBtn;
    QPushButton *m_applyBtn;
    QCheckBox *m_logToConsoleCheckBox;

    void createUI();
    void loadFromManager();
    void saveToManager();
};