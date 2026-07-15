#include "mainwindow.h"

#include "src/parser/ItfParser.h"
#include "src/ui/StudyBrowserWidget.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    mStudyBrowserWidget = new StudyBrowserWidget(this);
    setCentralWidget(mStudyBrowserWidget);

    createMenus();

    setWindowTitle("TA Comparator");
    resize(1000, 700);

    statusBar()->showMessage("Open a primary ITF file to begin.");
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu("File");

    mOpenPrimaryFileAction =
        new QAction("Open Primary ITF File...", this);

    mOpenCompareFileAction =
        new QAction("Open Compare ITF File...", this);

    connect(mOpenPrimaryFileAction,
            &QAction::triggered,
            this,
            &MainWindow::openPrimaryItfFile);

    connect(mOpenCompareFileAction,
            &QAction::triggered,
            this,
            &MainWindow::openCompareItfFile);

    fileMenu->addAction(mOpenPrimaryFileAction);
    fileMenu->addAction(mOpenCompareFileAction);
}

void MainWindow::openPrimaryItfFile()
{
    const QString fileName =
        QFileDialog::getOpenFileName(
            this,
            "Open Primary ITF File",
            QString(),
            "ITF Files (*.itf);;All Files (*.*)");

    if (fileName.isEmpty())
        return;

    Study loadedStudy;

    if (!loadStudyFromFile(fileName, loadedStudy))
        return;

    mPrimaryStudy = loadedStudy;
    mHasPrimaryStudy = true;

    refreshBrowser();

    const QFileInfo fileInfo(fileName);

    setWindowTitle(QString("TA Comparator - %1")
                       .arg(fileInfo.fileName()));

    statusBar()->showMessage(QString("Loaded primary file: %1")
                                 .arg(fileInfo.fileName()));
}

void MainWindow::openCompareItfFile()
{
    if (!mHasPrimaryStudy)
    {
        QMessageBox::information(
            this,
            "Primary File Required",
            "Please open a primary ITF file before opening a compare file.");

        return;
    }

    const QString fileName =
        QFileDialog::getOpenFileName(
            this,
            "Open Compare ITF File",
            QString(),
            "ITF Files (*.itf);;All Files (*.*)");

    if (fileName.isEmpty())
        return;

    Study loadedStudy;

    if (!loadStudyFromFile(fileName, loadedStudy))
        return;

    mCompareStudy = loadedStudy;
    mHasCompareStudy = true;

    refreshBrowser();

    const QFileInfo fileInfo(fileName);

    statusBar()->showMessage(QString("Loaded compare file: %1")
                                 .arg(fileInfo.fileName()));
}

bool MainWindow::loadStudyFromFile(const QString& fileName,
                                   Study& targetStudy)
{
    ItfParser parser;

    if (!parser.parse(fileName, targetStudy))
    {
        QMessageBox::critical(
            this,
            "Parse Failed",
            QString("Failed to parse ITF file:\n%1")
                .arg(fileName));

        statusBar()->showMessage("Failed to parse ITF file.");

        return false;
    }

    return true;
}

void MainWindow::refreshBrowser()
{
    const Study* primary =
        mHasPrimaryStudy ? &mPrimaryStudy : nullptr;

    const Study* compare =
        mHasCompareStudy ? &mCompareStudy : nullptr;

    mStudyBrowserWidget->setStudies(primary, compare);
}
