#pragma once

#include <QMainWindow>
#include <QString>
#include <QVector>

#include "src/model/Study.h"
#include "src/ui/PlotDisplayMapper.h"

class QAction;
class QDockWidget;
class FileSelectionWidget;
class PlotBrowserWidget;
class QActionGroup;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void addStudyFiles();
    void clearStudyFiles();

    void addPlotWindow();
    void resetPlotLayout();

    void onFileSelectionSettingsChanged();

    void showQuickHelp();
    void showAbout();

	void exportAllPlotsAsPng();
	void exportAllPlotsAsPdf();
	void exportSelectedPlotAsPng();

	void useIpsaFriendlyNames();
	void useRawItfNames();

private:
    void createMenus();

    bool loadStudyFromFile(const QString& fileName,
                           Study& targetStudy);

    void refreshBrowser();

    void setupDockUi();

    void refreshWidgets();
    void refreshPlotFileSettings();

    QVector<const Study*> studyPointers() const;

    void arrangePlotDocks();
	void removePlotWindow(QDockWidget* plotDock);
	void renumberPlotDocks();

	void warnIfLoadedNetworksDiffer();

	void applyNameDisplayMode();

	void equalizePlotDockSizes();

	void dumpDockLayout(const QString& label) const;

	void finalizePlotDockLayout(const QRect& previousGeometry,
                            bool wasMaximized);

private:
    QVector<Study> mStudies;

    QAction* mAddFilesAction = nullptr;
    QAction* mClearFilesAction = nullptr;

    QAction* mAddPlotWindowAction = nullptr;
    QAction* mResetPlotLayoutAction = nullptr;

    QAction* mQuickHelpAction = nullptr;
    QAction* mAboutAction = nullptr;

    FileSelectionWidget* mFileSelectionWidget = nullptr;
    QDockWidget* mFileSelectionDock = nullptr;

	QAction* mExportAllPlotsAsPngAction = nullptr;
	QAction* mExportAllPlotsAsPdfAction = nullptr;
	QAction* mExportSelectedPlotAsPngAction = nullptr;

    QVector<PlotBrowserWidget*> mPlotBrowserWidgets;
    QVector<QDockWidget*> mPlotDocks;

	QActionGroup* mNameDisplayActionGroup = nullptr;
	QAction* mIpsaFriendlyNamesAction = nullptr;
	QAction* mRawItfNamesAction = nullptr;
	
	NameDisplayMode mNameDisplayMode = NameDisplayMode::IpsaFriendly;

};
