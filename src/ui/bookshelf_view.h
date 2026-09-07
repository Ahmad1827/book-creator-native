#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include "types.h"
#include "constants.h"

class BookshelfView : public QWidget {
    Q_OBJECT

public:
    explicit BookshelfView(QWidget *parent = nullptr);

    void setProjects(const QList<BookProject> &projects);

signals:
    void openProjectRequested(const BookProject &project);
    void createProjectRequested();
    void loadProjectRequested();
    void deleteProjectRequested(const QString &projectId);
    void duplicateProjectRequested(const BookProject &project);
    void exportJsonRequested(const BookProject &project);

private:
    void setupUi();
    void refreshGrid();

    QLineEdit *m_editSearch;
    QWidget *m_gridContainer;
    QGridLayout *m_gridLayout;
    QLabel *m_lblCount;

    QList<BookProject> m_projects;
};