#pragma once

#include <QDialog>
#include <QRadioButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private:
    void refreshDetectedPath();
    void testClaude();
    void save();

    QRadioButton *m_radioOff;
    QRadioButton *m_radioClaudeCode;
    QLineEdit *m_editClaudePath;
    QComboBox *m_comboModel;
    QComboBox *m_comboAge;
    QLabel *m_lblStatus;
};
