#pragma once

#include <QMainWindow>
#include <QString>

#include "src/model/Study.h"

class QAction;
class StudyBrowserWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openPrimaryItfFile();
    void openCompareItfFile();

private:
    void createMenus();

    bool loadStudyFromFile(const QString& fileName,
                           Study& targetStudy);

    void refreshBrowser();

private:
    Study mPrimaryStudy;
    Study mCompareStudy;

    bool mHasPrimaryStudy = false;
    bool mHasCompareStudy = false;

    StudyBrowserWidget* mStudyBrowserWidget = nullptr;

    QAction* mOpenPrimaryFileAction = nullptr;
    QAction* mOpenCompareFileAction = nullptr;
};
