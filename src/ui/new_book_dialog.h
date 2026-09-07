#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QButtonGroup>
#include "types.h"
#include "constants.h"

class NewBookDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewBookDialog(QWidget *parent = nullptr);
    BookProject createProject() const;

private:
    void setupUi();

    QLineEdit *m_editTitle;
    QLineEdit *m_editAuthor;
    QString m_selectedThemeId = "botanical_meadow";
    QButtonGroup *m_themeGroup;
};