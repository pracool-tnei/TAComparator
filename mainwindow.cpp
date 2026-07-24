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
#include <QStringList>

#include "src/ui/FileSelectionWidget.h"
#include "src/ui/PlotBrowserWidget.h"
#include "src/ui/PlotDockWidget.h"

#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QWidget> 

#include <QMessageBox>

#include <QEvent>
#include <QTimer>

#include <QDir>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPixmap>
#include <QRegularExpression>
#include <QInputDialog>

#include <QActionGroup>

namespace
{
    constexpr int MaxComparisonFiles = 5;
	const QSize ExportPlotImageSize(1600, 900);

	QRect centeredPixmapRect(const QRect& pageRect,
							 const QSize& pixmapSize)
	{
		if (pageRect.isEmpty() ||
			!pixmapSize.isValid() ||
			pixmapSize.width() <= 0 ||
			pixmapSize.height() <= 0)
		{
			return QRect();
		}
	
		/*
		 * Important:
		 * Use only the printable area's size, not its x/y offset.
		 *
		 * QPdfWriter painting is already inside the page coordinate system.
		 * Using paintRectPixels().x() / y() here can shift the image right/down.
		 */
		QRect availableRect(QPoint(0, 0),
							pageRect.size());
	
		const int horizontalPadding =
			qMax(40,
				 availableRect.width() / 35);
	
		const int verticalPadding =
			qMax(30,
				 availableRect.height() / 35);
	
		QRect fitRect =
			availableRect.adjusted(horizontalPadding,
								   verticalPadding,
								   -horizontalPadding,
								   -verticalPadding);
	
		QSize scaledSize =
			pixmapSize;
	
		scaledSize.scale(fitRect.size(),
						 Qt::KeepAspectRatio);
	
		const int x =
			fitRect.x() +
			(fitRect.width() - scaledSize.width()) / 2;
	
		const int y =
			fitRect.y() +
			(fitRect.height() - scaledSize.height()) / 2;
	
		return QRect(QPoint(x, y),
					 scaledSize);
	}

	QString safeExportFileName(const QString& text)
	{
	    QString safeName =
	        text.trimmed();

	    if (safeName.isEmpty())
	    {
	        safeName = "Plot";
	    }

	    safeName.replace(QRegularExpression("[\\\\/:*?\"<>|]+"),
	                     "_");

	    safeName.replace(QRegularExpression("\\s+"),
	                     "_");

	    while (safeName.contains("__"))
	    {
	        safeName.replace("__", "_");
	    }

	    if (safeName.size() > 120)
	    {
	        safeName = safeName.left(120);
	    }

	    return safeName;
	}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    QWidget* emptyCentralWidget = new QWidget(this);
    emptyCentralWidget->setMinimumSize(0, 0);
    emptyCentralWidget->setMaximumSize(0, 0);

    setCentralWidget(emptyCentralWidget);

    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::AnimatedDocks);

    setupDockUi();

    createMenus();

    addPlotWindow();

    setWindowTitle("TA Comparator");
}

void MainWindow::setupDockUi()
{
    mFileSelectionWidget =
        new FileSelectionWidget(this);

    mFileSelectionWidget->setMinimumWidth(260);

    mFileSelectionDock =
        new QDockWidget("Files", this);

    mFileSelectionDock->setObjectName("FilesDock");
    mFileSelectionDock->setWidget(mFileSelectionWidget);

    mFileSelectionDock->setMinimumWidth(280);
    mFileSelectionDock->setMaximumWidth(380);

    mFileSelectionDock->setAllowedAreas(Qt::LeftDockWidgetArea |
                                        Qt::RightDockWidgetArea);

	mFileSelectionDock->setFeatures(
		QDockWidget::DockWidgetMovable |
		QDockWidget::DockWidgetFloatable |
		QDockWidget::DockWidgetClosable);


    addDockWidget(Qt::LeftDockWidgetArea,
                  mFileSelectionDock);

    connect(mFileSelectionWidget,
            &FileSelectionWidget::settingsChanged,
            this,
            &MainWindow::onFileSelectionSettingsChanged);

	connect(mFileSelectionWidget,
			&FileSelectionWidget::removeStudyRequested,
			this,
			&MainWindow::removeStudyFile);

}

void MainWindow::addPlotWindow()
{
	const bool shouldRestoreWindowGeometry =
    isVisible();

	const QRect previousGeometry =
	    geometry();

	const bool wasMaximized =
	    isMaximized();
	
    constexpr int MaxPlotWindows = 4;

    if (mPlotBrowserWidgets.size() >= MaxPlotWindows)
    {
        QMessageBox::information(
            this,
            "Maximum plots",
            "A maximum of 4 plot windows is supported.");

        return;
    }

    const int plotNumber =
        mPlotBrowserWidgets.size() + 1;

    PlotBrowserWidget* plotWidget =
        new PlotBrowserWidget(this);
	
	plotWidget->setNameDisplayMode(mNameDisplayMode);

	connect(plotWidget,
			&PlotBrowserWidget::plotXRangeChanged,
			this,
			&MainWindow::onPlotXRangeChanged);


    PlotDockWidget* plotDock =
    new PlotDockWidget(
        QString("Plot %1").arg(plotNumber),
        this);

    plotDock->setObjectName(
        QString("PlotDock%1").arg(plotNumber));

    plotDock->setWidget(plotWidget);

    plotDock->setAllowedAreas(Qt::AllDockWidgetAreas);

    plotDock->setFeatures(
    QDockWidget::DockWidgetMovable |
    QDockWidget::DockWidgetFloatable |
    QDockWidget::DockWidgetClosable);

	connect(plotDock,
        &PlotDockWidget::closeRequested,
        this,
        [this](PlotDockWidget* dock)
        {
            removePlotWindow(dock);
        });

    mPlotBrowserWidgets.append(plotWidget);
    mPlotDocks.append(plotDock);

    addDockWidget(Qt::RightDockWidgetArea,
                  plotDock);

    const QVector<const Study*> pointers =
        studyPointers();

    plotWidget->setStudies(pointers);

    if (mFileSelectionWidget)
    {
        plotWidget->setFilePlotSettings(
            mFileSelectionWidget->filePlotSettings());
    }

    arrangePlotDocks();

	if (shouldRestoreWindowGeometry)
	{
	    finalizePlotDockLayout(previousGeometry,
	                           wasMaximized);
	}
}

void MainWindow::resetPlotLayout()
{
    if (mPlotDocks.isEmpty())
    {
        return;
    }

    setUpdatesEnabled(false);

    //
    // Keep Files dock stable. Do not remove/rebuild it aggressively.
    //
    if (mFileSelectionDock)
    {
        mFileSelectionDock->setFloating(false);

        addDockWidget(Qt::LeftDockWidgetArea,
                      mFileSelectionDock);

        mFileSelectionDock->show();
        mFileSelectionDock->raise();
    }

    //
    // Fully detach plot docks first.
    // Do not immediately add all of them back, because that lets Qt create
    // a temporary stacked layout.
    //
    for (QDockWidget* dock : mPlotDocks)
    {
        if (!dock)
        {
            continue;
        }

        dock->setFloating(false);
        dock->hide();

        removeDockWidget(dock);
    }

    //
    // Rebuild the plot layout directly into the intended shape:
    //
    // 1 plot:
    //   1
    //
    // 2 plots:
    //   1 | 2
    //
    // 3 plots:
    //   1 | 2
    //   3 |
    //
    // 4 plots:
    //   1 | 2
    //   3 | 4
    //
    QDockWidget* plot1 =
        mPlotDocks.value(0, nullptr);

    if (!plot1)
    {
        setUpdatesEnabled(true);
        return;
    }

    plot1->show();

    addDockWidget(Qt::RightDockWidgetArea,
                  plot1);

	if (mPlotDocks.size() >= 3 &&
		mPlotDocks[2])
	{
		mPlotDocks[2]->show();
	
		//
		// First create the two rows.
		//
		splitDockWidget(plot1,
						mPlotDocks[2],
						Qt::Vertical);
	}
	
	if (mPlotDocks.size() >= 2 &&
		mPlotDocks[1])
	{
		mPlotDocks[1]->show();
	
		//
		// Then split the top row.
		//
		splitDockWidget(plot1,
						mPlotDocks[1],
						Qt::Horizontal);
	}
	
	if (mPlotDocks.size() >= 4 &&
		mPlotDocks[3])
	{
		mPlotDocks[3]->show();
	
		//
		// Then split the bottom row.
		//
		splitDockWidget(mPlotDocks[2],
						mPlotDocks[3],
						Qt::Horizontal);
	}

    for (QDockWidget* dock : mPlotDocks)
    {
        if (dock)
        {
            dock->show();
            dock->raise();
        }
    }

    setUpdatesEnabled(true);

    //
    // Let Qt complete the dock split first, then resize.
    //
    QTimer::singleShot(
        0,
        this,
        [this]()
        {
            if (mFileSelectionDock &&
                !mPlotDocks.isEmpty() &&
                mPlotDocks[0])
            {
                resizeDocks({ mFileSelectionDock, mPlotDocks[0] },
                            { 300, 1200 },
                            Qt::Horizontal);
            }

            if (mPlotDocks.size() == 2)
            {
                resizeDocks({ mPlotDocks[0], mPlotDocks[1] },
                            { 1, 1 },
                            Qt::Horizontal);
            }
            else if (mPlotDocks.size() == 3)
            {
                resizeDocks({ mPlotDocks[0], mPlotDocks[2] },
                            { 1, 1 },
                            Qt::Vertical);

                resizeDocks({ mPlotDocks[0], mPlotDocks[1] },
                            { 1, 1 },
                            Qt::Horizontal);
            }
            else if (mPlotDocks.size() >= 4)
            {
                resizeDocks({ mPlotDocks[0], mPlotDocks[2] },
                            { 1, 1 },
                            Qt::Vertical);

                resizeDocks({ mPlotDocks[0], mPlotDocks[1] },
                            { 1, 1 },
                            Qt::Horizontal);

                resizeDocks({ mPlotDocks[2], mPlotDocks[3] },
                            { 1, 1 },
                            Qt::Horizontal);
            }
			dumpDockLayout("AFTER resetPlotLayout final");
        });
}

void MainWindow::equalizePlotDockSizes()
{
    if (mPlotDocks.isEmpty())
    {
        return;
    }

    for (QDockWidget* dock : mPlotDocks)
    {
        if (dock)
        {
            dock->show();
            dock->raise();
        }
    }

    //
    // Keep Files dock at a reasonable width if it is visible.
    //
    if (mFileSelectionDock &&
        mFileSelectionDock->isVisible())
    {
        resizeDocks({ mFileSelectionDock, mPlotDocks[0] },
                    { 300, 1200 },
                    Qt::Horizontal);
    }

    if (mPlotDocks.size() == 1)
    {
        return;
    }

    if (mPlotDocks.size() == 2)
    {
        resizeDocks({ mPlotDocks[0], mPlotDocks[1] },
                    { 1, 1 },
                    Qt::Horizontal);

        return;
    }

    if (mPlotDocks.size() == 3)
    {
        resizeDocks({ mPlotDocks[0], mPlotDocks[2] },
                    { 1, 1 },
                    Qt::Vertical);

        resizeDocks({ mPlotDocks[0], mPlotDocks[1] },
                    { 1, 1 },
                    Qt::Horizontal);

        return;
    }

    if (mPlotDocks.size() == 4)
    {
        resizeDocks({ mPlotDocks[0], mPlotDocks[2] },
                    { 1, 1 },
                    Qt::Vertical);

        resizeDocks({ mPlotDocks[0], mPlotDocks[1] },
                    { 1, 1 },
                    Qt::Horizontal);

        resizeDocks({ mPlotDocks[2], mPlotDocks[3] },
                    { 1, 1 },
                    Qt::Horizontal);

        return;
    }
}

void MainWindow::finalizePlotDockLayout(const QRect& previousGeometry,
                                        bool wasMaximized)
{
    /*
     * Qt dock layout recalculation can temporarily increase the MainWindow
     * minimum size. Let Qt complete one layout pass first, then restore the
     * user-visible window size.
     */
    QTimer::singleShot(
        0,
        this,
        [this, previousGeometry, wasMaximized]()
        {
            equalizePlotDockSizes();

            if (wasMaximized)
            {
                showMaximized();
            }
            else
            {
                setGeometry(previousGeometry);
            }

            /*
             * Run one more equalization after the window size has been restored.
             */
            QTimer::singleShot(
                0,
                this,
                [this]()
                {
                    equalizePlotDockSizes();
                });
        });
}

void MainWindow::arrangePlotDocks()
{
    if (mPlotDocks.isEmpty())
    {
        return;
    }

    for (QDockWidget* dock : mPlotDocks)
    {
        if (dock)
        {
            dock->show();
            dock->raise();
        }
    }

    if (mPlotDocks.size() == 1)
    {
        return;
    }

    if (mPlotDocks.size() == 2)
    {
        splitDockWidget(mPlotDocks[0],
                        mPlotDocks[1],
                        Qt::Horizontal);

        return;
    }

    if (mPlotDocks.size() == 3)
    {
        splitDockWidget(mPlotDocks[0],
                        mPlotDocks[2],
                        Qt::Vertical);

        splitDockWidget(mPlotDocks[0],
                        mPlotDocks[1],
                        Qt::Horizontal);

        resizeDocks({ mPlotDocks[0], mPlotDocks[2] },
                    { 600, 600 },
                    Qt::Vertical);

        resizeDocks({ mPlotDocks[0], mPlotDocks[1] },
                    { 600, 600 },
                    Qt::Horizontal);

        return;
    }

    if (mPlotDocks.size() == 4)
    {
        splitDockWidget(mPlotDocks[0],
                        mPlotDocks[2],
                        Qt::Vertical);

        splitDockWidget(mPlotDocks[0],
                        mPlotDocks[1],
                        Qt::Horizontal);

        splitDockWidget(mPlotDocks[2],
                        mPlotDocks[3],
                        Qt::Horizontal);

        resizeDocks({ mPlotDocks[0], mPlotDocks[2] },
                    { 600, 600 },
                    Qt::Vertical);

        resizeDocks({ mPlotDocks[0], mPlotDocks[1] },
                    { 600, 600 },
                    Qt::Horizontal);

        resizeDocks({ mPlotDocks[2], mPlotDocks[3] },
                    { 600, 600 },
                    Qt::Horizontal);

        return;
    }
}

void MainWindow::removePlotWindow(QDockWidget* plotDock)
{
    if (!plotDock)
    {
        return;
    }

    if (mPlotDocks.size() <= 1)
    {
        QMessageBox::information(
            this,
            "Cannot close plot",
            "At least one plot window must remain.");

        plotDock->show();

        if (mFileSelectionDock)
        {
            mFileSelectionDock->setFloating(false);
            mFileSelectionDock->show();
            mFileSelectionDock->raise();
        }

        return;
    }

    const int index =
        mPlotDocks.indexOf(plotDock);

    if (index < 0)
    {
        return;
    }

    mPlotDocks.removeAt(index);

    if (index < mPlotBrowserWidgets.size())
    {
        mPlotBrowserWidgets.removeAt(index);
    }

    removeDockWidget(plotDock);

    plotDock->deleteLater();

    renumberPlotDocks();

    /*
     * Important:
     * Do not call resetPlotLayout() here.
     * Closing a plot should not remove/re-add the Files dock.
     */
    if (mFileSelectionDock)
    {
        mFileSelectionDock->setFloating(false);
        mFileSelectionDock->show();
        mFileSelectionDock->raise();

        addDockWidget(Qt::LeftDockWidgetArea,
                      mFileSelectionDock);
    }

    for (QDockWidget* dock : mPlotDocks)
    {
        if (!dock)
        {
            continue;
        }

        dock->show();
        addDockWidget(Qt::RightDockWidgetArea,
                      dock);
    }

    arrangePlotDocks();

    if (mFileSelectionDock && !mPlotDocks.isEmpty())
    {
        resizeDocks({ mFileSelectionDock, mPlotDocks[0] },
                    { 300, 1200 },
                    Qt::Horizontal);
    }
}

void MainWindow::renumberPlotDocks()
{
    for (int i = 0; i < mPlotDocks.size(); ++i)
    {
        QDockWidget* dock =
            mPlotDocks[i];

        if (!dock)
        {
            continue;
        }

        const int plotNumber =
            i + 1;

        dock->setWindowTitle(
            QString("Plot %1").arg(plotNumber));

        dock->setObjectName(
            QString("PlotDock%1").arg(plotNumber));
    }
}

void MainWindow::showQuickHelp()
{
    QMessageBox::information(
        this,
        "TA Comparator - Quick Help",
        "TA Comparator Quick Help\n\n"
        "File menu:\n"
        "- Add ITF File(s): Load one or more transient result files.\n"
        "- Clear Loaded Files: Remove all loaded files from the current session.\n\n"
        "Files panel:\n"
        "- Tick files to include them in all plot windows.\n"
        "- Choose color and line thickness per file.\n\n"
        "Plot menu:\n"
        "- Add Plot Window: Add up to 4 plot windows.\n"
        "- Reset Plot Layout: Restore the 2 x 2 plot layout.\n\n"
        "Each plot window:\n"
        "- Select Study Type, Component, Signal and Plot Type independently.\n"
        "- Use mouse wheel to zoom on the time axis.\n"
        "- Drag inside a zoomed plot to pan.\n"
        "- Double-click a plot to reset zoom.\n"
        "- Hover over a plot to compare values across selected files.");
}

void MainWindow::showAbout()
{
    QMessageBox::about(
        this,
        "About TA Comparator",
        "TA Comparator\n\n"
        "A Qt-based tool for comparing IPSA transient analysis ITF files.\n\n"
        "Features:\n"
        "- Multiple ITF file comparison\n"
        "- Independent dockable plot windows\n"
        "- Shared file selection, color and thickness settings\n"
        "- Line and bar plots\n"
        "- Hover comparison tooltip\n"
        "- Zoom and pan support\n"
        "- Resettable 2 x 2 plot layout");
}

void MainWindow::exportAllPlotsAsPng()
{
    if (mPlotBrowserWidgets.isEmpty())
    {
        QMessageBox::information(
            this,
            "No plots",
            "There are no plots to export.");

        return;
    }

    const QString exportFolder =
        QFileDialog::getExistingDirectory(
            this,
            "Export All Plots as PNG Files");

    if (exportFolder.isEmpty())
    {
        return;
    }

    QDir directory(exportFolder);

    int exportedCount = 0;
    QStringList failedFiles;

    for (int i = 0; i < mPlotBrowserWidgets.size(); ++i)
    {
        PlotBrowserWidget* plotBrowser =
            mPlotBrowserWidgets[i];

        if (!plotBrowser)
        {
            continue;
        }

        const QString plotTitle =
            plotBrowser->exportPlotTitle();

        const QString fileName =
            QString("Plot_%1_%2.png")
                .arg(i + 1)
                .arg(safeExportFileName(plotTitle));

        const QString filePath =
            directory.filePath(fileName);

		const QSize exportPlotSize(1600, 900);
		
		QPixmap pixmap =
			plotBrowser->exportPlotPixmap(exportPlotSize);


        if (pixmap.isNull())
        {
            failedFiles.append(fileName);
            continue;
        }

        if (!pixmap.save(filePath, "PNG"))
        {
            failedFiles.append(fileName);
            continue;
        }

        ++exportedCount;
    }

    QString message =
        QString("Exported %1 plot(s) as PNG files.")
            .arg(exportedCount);

    if (!failedFiles.isEmpty())
    {
        message += "\n\nFailed files:\n";

        for (const QString& failedFile : failedFiles)
        {
            message += "  - " + failedFile + "\n";
        }
    }

    QMessageBox::information(
        this,
        "Export complete",
        message);
}

void MainWindow::exportAllPlotsAsPdf()
{
    if (mPlotBrowserWidgets.isEmpty())
    {
        QMessageBox::information(this,
                                 "No plots",
                                 "There are no plots to export.");

        return;
    }

    const QString fileName =
        QFileDialog::getSaveFileName(
            this,
            "Export All Plots as PDF",
            QString(),
            "PDF Files (*.pdf)");

    if (fileName.isEmpty())
    {
        return;
    }

    QPdfWriter pdfWriter(fileName);

    pdfWriter.setResolution(300);

    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setPageOrientation(QPageLayout::Landscape);

    pdfWriter.setPageMargins(QMarginsF(12, 12, 12, 12),
                             QPageLayout::Millimeter);

    QPainter painter(&pdfWriter);

    if (!painter.isActive())
    {
        QMessageBox::critical(this,
                              "Export Failed",
                              "Unable to create PDF file.");

        return;
    }

    bool firstPage =
        true;

    for (PlotBrowserWidget* plotWidget : mPlotBrowserWidgets)
    {
        if (!plotWidget)
        {
            continue;
        }

        const QPixmap plotPixmap =
            plotWidget->exportPlotPixmap(ExportPlotImageSize);

        if (plotPixmap.isNull())
        {
            continue;
        }

        if (!firstPage)
        {
            pdfWriter.newPage();
        }

        firstPage =
            false;

        const QRect pageRect =
            pdfWriter.pageLayout()
                .paintRectPixels(pdfWriter.resolution());

        const QRect targetRect =
            centeredPixmapRect(pageRect,
                               plotPixmap.size());

        painter.drawPixmap(targetRect,
                           plotPixmap);
    }

    painter.end();

    statusBar()->showMessage(
        QString("Exported PDF: %1")
            .arg(fileName));
}

void MainWindow::exportSelectedPlotAsPng()
{
    if (mPlotBrowserWidgets.isEmpty())
    {
        QMessageBox::information(
            this,
            "No plots",
            "There are no plots to export.");

        return;
    }

    QStringList plotChoices;
    QVector<int> plotIndexes;

    for (int i = 0; i < mPlotBrowserWidgets.size(); ++i)
    {
        PlotBrowserWidget* plotBrowser =
            mPlotBrowserWidgets[i];

        if (!plotBrowser)
        {
            continue;
        }

        const QString choiceText =
            QString("Plot %1 - %2")
                .arg(i + 1)
                .arg(plotBrowser->exportPlotTitle());

        plotChoices.append(choiceText);
        plotIndexes.append(i);
    }

    if (plotChoices.isEmpty())
    {
        QMessageBox::information(
            this,
            "No plots",
            "There are no valid plots to export.");

        return;
    }

    bool ok = false;

    const QString selectedPlot =
        QInputDialog::getItem(
            this,
            "Export Selected Plot",
            "Select plot to export:",
            plotChoices,
            0,
            false,
            &ok);

    if (!ok || selectedPlot.isEmpty())
    {
        return;
    }

    const int selectedChoiceIndex =
        plotChoices.indexOf(selectedPlot);

    if (selectedChoiceIndex < 0 ||
        selectedChoiceIndex >= plotIndexes.size())
    {
        return;
    }

    const int plotIndex =
        plotIndexes[selectedChoiceIndex];

    PlotBrowserWidget* plotBrowser =
        mPlotBrowserWidgets[plotIndex];

    if (!plotBrowser)
    {
        return;
    }

    const QString defaultFileName =
        QString("Plot_%1_%2.png")
            .arg(plotIndex + 1)
            .arg(safeExportFileName(
                plotBrowser->exportPlotTitle()));

    QString filePath =
        QFileDialog::getSaveFileName(
            this,
            "Export Selected Plot as PNG",
            defaultFileName,
            "PNG Files (*.png)");

    if (filePath.isEmpty())
    {
        return;
    }

    if (!filePath.endsWith(".png",
                           Qt::CaseInsensitive))
    {
        filePath += ".png";
    }

	const QSize exportPlotSize(1600, 900);
	
	QPixmap pixmap =
		plotBrowser->exportPlotPixmap(exportPlotSize);


    if (pixmap.isNull())
    {
        QMessageBox::critical(
            this,
            "Export failed",
            "Could not render the selected plot.");

        return;
    }

    if (!pixmap.save(filePath, "PNG"))
    {
        QMessageBox::critical(
            this,
            "Export failed",
            "Could not save the selected plot as PNG.");

        return;
    }

    QMessageBox::information(
        this,
        "Export complete",
        QString("Exported Plot %1 as PNG.")
            .arg(plotIndex + 1));
}

void MainWindow::useIpsaFriendlyNames()
{
    mNameDisplayMode =
        NameDisplayMode::IpsaFriendly;

    applyNameDisplayMode();
}

void MainWindow::useRawItfNames()
{
    mNameDisplayMode =
        NameDisplayMode::Raw;

    applyNameDisplayMode();
}

void MainWindow::applyNameDisplayMode()
{
    for (PlotBrowserWidget* plotWidget : mPlotBrowserWidgets)
    {
        if (!plotWidget)
        {
            continue;
        }

        plotWidget->setNameDisplayMode(mNameDisplayMode);
    }
}

QVector<const Study*> MainWindow::studyPointers() const
{
    QVector<const Study*> pointers;

    for (const Study& study : mStudies)
    {
        pointers.append(&study);
    }

    return pointers;
}

void MainWindow::refreshWidgets()
{
    const QVector<const Study*> pointers =
        studyPointers();

    /*
     * Rebuild the Files panel first.
     * Block signals so it does not trigger plot refresh halfway through.
     */
    if (mFileSelectionWidget)
    {
        const bool previousSignalState =
            mFileSelectionWidget->blockSignals(true);

        mFileSelectionWidget->setStudies(pointers);

        mFileSelectionWidget->blockSignals(previousSignalState);
    }

    QVector<FilePlotSettings> settings;

    if (!pointers.isEmpty() &&
        mFileSelectionWidget)
    {
        settings =
            mFileSelectionWidget->filePlotSettings();
    }

    for (PlotBrowserWidget* plotWidget : mPlotBrowserWidgets)
    {
        if (!plotWidget)
        {
            continue;
        }

        /*
         * Important:
         * Set the new Study pointers first.
         *
         * setFilePlotSettings() calls updatePlot().
         * If it is called before setStudies(), the plot may still contain
         * stale Study pointers from before mStudies.removeAt().
         */
        plotWidget->setStudies(pointers);
        plotWidget->setFilePlotSettings(settings);
    }
}

void MainWindow::refreshPlotFileSettings()
{
    if (!mFileSelectionWidget)
    {
        return;
    }

    const QVector<FilePlotSettings> settings =
        mFileSelectionWidget->filePlotSettings();

    for (PlotBrowserWidget* plotWidget : mPlotBrowserWidgets)
    {
        if (plotWidget)
        {
            plotWidget->setFilePlotSettings(settings);
        }
    }
}

void MainWindow::onFileSelectionSettingsChanged()
{
    refreshPlotFileSettings();
}

void MainWindow::createMenus()
{
    menuBar()->clear();

    //
    // File menu first.
    //
    QMenu* fileMenu =
        menuBar()->addMenu("File");

    mAddFilesAction =
        fileMenu->addAction("Add ITF File(s)...");

    connect(mAddFilesAction,
            &QAction::triggered,
            this,
            &MainWindow::addStudyFiles);

    mClearFilesAction =
        fileMenu->addAction("Clear Loaded Files");

    connect(mClearFilesAction,
            &QAction::triggered,
            this,
            &MainWindow::clearStudyFiles);

    fileMenu->addSeparator();

    QAction* exitAction =
        fileMenu->addAction("Exit");

    connect(exitAction,
            &QAction::triggered,
            this,
            &QWidget::close);

    //
    // Plot menu second.
    //
    QMenu* plotMenu =
        menuBar()->addMenu("Plot");

	mAddPlotWindowAction =
		plotMenu->addAction("Add Plot Window");
	
	connect(mAddPlotWindowAction,
			&QAction::triggered,
			this,
			&MainWindow::addPlotWindow);
	
	mResetPlotLayoutAction =
		plotMenu->addAction("Reset Plot Window Layout");
	
	connect(mResetPlotLayoutAction,
			&QAction::triggered,
			this,
			&MainWindow::resetPlotLayout);

	//
	// View menu third.
	//
	QMenu* viewMenu =
		menuBar()->addMenu("View");
	
	QMenu* nameDisplayMenu =
		viewMenu->addMenu("Name Display");
	
	mNameDisplayActionGroup =
		new QActionGroup(this);
	
	mNameDisplayActionGroup->setExclusive(true);
	
	mIpsaFriendlyNamesAction =
		nameDisplayMenu->addAction("IPSA Friendly Names");
	
	mIpsaFriendlyNamesAction->setCheckable(true);
	mIpsaFriendlyNamesAction->setChecked(
		mNameDisplayMode == NameDisplayMode::IpsaFriendly);
	
	mRawItfNamesAction =
		nameDisplayMenu->addAction("Raw ITF Names");
	
	mRawItfNamesAction->setCheckable(true);
	mRawItfNamesAction->setChecked(
		mNameDisplayMode == NameDisplayMode::Raw);
	
	mNameDisplayActionGroup->addAction(mIpsaFriendlyNamesAction);
	mNameDisplayActionGroup->addAction(mRawItfNamesAction);
	
	connect(mIpsaFriendlyNamesAction,
			&QAction::triggered,
			this,
			&MainWindow::useIpsaFriendlyNames);
	
	connect(mRawItfNamesAction,
			&QAction::triggered,
			this,
			&MainWindow::useRawItfNames);

	//hide file selection
	viewMenu->addSeparator();
	
	mFilesPanelAction =
		viewMenu->addAction("Files Panel");
	
	mFilesPanelAction->setCheckable(true);
	mFilesPanelAction->setChecked(
		mFileSelectionDock &&
		mFileSelectionDock->isVisible());
	
	connect(mFilesPanelAction,
			&QAction::triggered,
			this,
			[this](bool checked)
			{
				if (!mFileSelectionDock)
				{
					return;
				}
	
				mFileSelectionDock->setVisible(checked);
	
				if (checked)
				{
					addDockWidget(Qt::LeftDockWidgetArea,
								  mFileSelectionDock);
	
					mFileSelectionDock->raise();
				}
	
				QTimer::singleShot(
					0,
					this,
					[this]()
					{
						equalizePlotDockSizes();
					});
			});
	
	if (mFileSelectionDock)
	{
		connect(mFileSelectionDock,
				&QDockWidget::visibilityChanged,
				this,
				[this](bool visible)
				{
					if (mFilesPanelAction)
					{
						mFilesPanelAction->setChecked(visible);
					}
	
					QTimer::singleShot(
						0,
						this,
						[this]()
						{
							equalizePlotDockSizes();
						});
				});
	}

	viewMenu->addSeparator();
	
	mSyncPlotZoomAction =
		viewMenu->addAction("Sync Plot Zoom");
	
	mSyncPlotZoomAction->setCheckable(true);
	mSyncPlotZoomAction->setChecked(mSyncPlotZoom);
	
	connect(mSyncPlotZoomAction,
			&QAction::toggled,
			this,
			[this](bool checked)
			{
				mSyncPlotZoom =
					checked;
			});


	//
	// Export menu fourth.
	//
	QMenu* exportMenu =
	    menuBar()->addMenu("Export");

	mExportSelectedPlotAsPngAction =
	    exportMenu->addAction("Export Selected Plot as PNG...");

	connect(mExportSelectedPlotAsPngAction,
	        &QAction::triggered,
	        this,
	        &MainWindow::exportSelectedPlotAsPng);

	exportMenu->addSeparator();

	mExportAllPlotsAsPngAction =
	    exportMenu->addAction("Export All Plots as PNG Files...");

	connect(mExportAllPlotsAsPngAction,
	        &QAction::triggered,
	        this,
	        &MainWindow::exportAllPlotsAsPng);

	mExportAllPlotsAsPdfAction =
	    exportMenu->addAction("Export All Plots as PDF Report...");

	connect(mExportAllPlotsAsPdfAction,
	        &QAction::triggered,
	        this,
	        &MainWindow::exportAllPlotsAsPdf);

    //
    // Help menu last.
    //
    QMenu* helpMenu =
        menuBar()->addMenu("Help");

    mQuickHelpAction =
        helpMenu->addAction("Quick Help");

    connect(mQuickHelpAction,
            &QAction::triggered,
            this,
            &MainWindow::showQuickHelp);

    mAboutAction =
        helpMenu->addAction("About");

    connect(mAboutAction,
            &QAction::triggered,
            this,
            &MainWindow::showAbout);
}

void MainWindow::onPlotXRangeChanged(PlotBrowserWidget* sourcePlot,
                                     double minX,
                                     double maxX,
                                     bool hasCustomRange)
{
    if (!mSyncPlotZoom)
    {
        return;
    }

    if (mIsApplyingSynchronizedZoom)
    {
        return;
    }

    if (!sourcePlot)
    {
        return;
    }

    mIsApplyingSynchronizedZoom =
        true;

    for (PlotBrowserWidget* plotWidget : mPlotBrowserWidgets)
    {
        if (!plotWidget ||
            plotWidget == sourcePlot)
        {
            continue;
        }

        plotWidget->applyExternalXRange(minX,
                                        maxX,
                                        hasCustomRange);
    }

    mIsApplyingSynchronizedZoom =
        false;
}

void MainWindow::addStudyFiles()
{
    const int remainingFiles =
        MaxComparisonFiles - static_cast<int>(mStudies.size());

    if (remainingFiles <= 0)
    {
        QMessageBox::information(
            this,
            "Maximum Files Loaded",
            QString("You can compare a maximum of %1 ITF files.")
                .arg(MaxComparisonFiles));

        return;
    }

    const QStringList selectedFiles =
        QFileDialog::getOpenFileNames(
            this,
            "Add ITF File(s)",
            QString(),
            "ITF Files (*.itf);;All Files (*.*)");

    if (selectedFiles.isEmpty())
    {
        return;
    }

    QStringList filesToLoad = selectedFiles;

    if (filesToLoad.size() > remainingFiles)
    {
        filesToLoad = filesToLoad.mid(0, remainingFiles);

        QMessageBox::information(
            this,
            "File Limit Applied",
            QString("Only %1 more file(s) can be added. "
                    "The extra selected files were ignored.")
                .arg(remainingFiles));
    }

    int loadedCount = 0;

    for (const QString& fileName : filesToLoad)
    {
        Study loadedStudy;

        if (!loadStudyFromFile(fileName, loadedStudy))
        {
            continue;
        }

        mStudies.append(loadedStudy);
        loadedCount++;
    }

    refreshBrowser();

    setWindowTitle(QString("TA Comparator - %1 file(s)")
                       .arg(mStudies.size()));

    statusBar()->showMessage(
        QString("Loaded %1 file(s). Total: %2.")
            .arg(loadedCount)
            .arg(mStudies.size()));

	if (loadedCount > 0)
	{
	    warnIfLoadedNetworksDiffer();
	}
}

void MainWindow::clearStudyFiles()
{
    /*
     * First detach all UI widgets from the current Study pointers.
     * This must happen before mStudies.clear().
     */
    const QVector<const Study*> emptyStudies;
    const QVector<FilePlotSettings> emptyFileSettings;

    for (PlotBrowserWidget* plotWidget : mPlotBrowserWidgets)
    {
        if (!plotWidget)
        {
            continue;
        }

        plotWidget->setStudies(emptyStudies);
        plotWidget->setFilePlotSettings(emptyFileSettings);
    }

    if (mFileSelectionWidget)
    {
        const bool previousSignalState =
            mFileSelectionWidget->blockSignals(true);

        mFileSelectionWidget->setStudies(emptyStudies);

        mFileSelectionWidget->blockSignals(previousSignalState);
    }

    mStudies.clear();

    setWindowTitle("TA Comparator");

    statusBar()->showMessage("Loaded files cleared.");
}

void MainWindow::removeStudyFile(int studyIndex)
{
    if (studyIndex < 0 ||
        studyIndex >= mStudies.size())
    {
        return;
    }

    const QString fileName =
        QFileInfo(mStudies[studyIndex].getFileName()).fileName();

    const QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            "Remove file",
            QString("Remove this file from comparison?\n\n%1")
                .arg(fileName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

	QStringList selectedFilesAfterRemove;
	
	if (mFileSelectionWidget)
	{
		const QVector<FilePlotSettings> currentSettings =
			mFileSelectionWidget->filePlotSettings();
	
		for (int i = 0; i < mStudies.size(); ++i)
		{
			if (i == studyIndex)
			{
				continue;
			}
	
			if (i >= currentSettings.size())
			{
				continue;
			}
	
			if (currentSettings[i].mEnabled)
			{
				selectedFilesAfterRemove.append(
					mStudies[i].getFileName());
			}
		}
	
		mFileSelectionWidget->setPendingSelectedFileNames(
			selectedFilesAfterRemove);
	}


    /*
     * Remove the actual loaded Study from MainWindow.
     * FileSelectionWidget only requested the removal;
     * it does not own the Study data.
     */
    mStudies.removeAt(studyIndex);

    refreshWidgets();

    if (mStudies.isEmpty())
    {
        setWindowTitle("TA Comparator");
    }
    else
    {
        setWindowTitle(
            QString("TA Comparator - %1 file(s)")
                .arg(mStudies.size()));
    }

    statusBar()->showMessage(
        QString("Removed file: %1")
            .arg(fileName));
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
    refreshWidgets();
}

void MainWindow::warnIfLoadedNetworksDiffer()
{
    QStringList uniqueNetworkNames;
    QStringList fileNetworkLines;

    for (const Study& study : mStudies)
    {
        const QString networkName =
            study.getNetworkName().trimmed();

        QString fileName;

        fileName = QFileInfo(study.getFileName()).fileName();

        const QString displayNetworkName =
            networkName.isEmpty() ? "(Unknown Network)" : networkName;

        fileNetworkLines.append(
            QString("  - %1  ->  %2")
                .arg(fileName, displayNetworkName));

        if (networkName.isEmpty())
        {
            continue;
        }

        bool alreadyKnown = false;

        for (const QString& knownNetworkName : uniqueNetworkNames)
        {
            if (QString::compare(knownNetworkName,
                                 networkName,
                                 Qt::CaseInsensitive) == 0)
            {
                alreadyKnown = true;
                break;
            }
        }

        if (!alreadyKnown)
        {
            uniqueNetworkNames.append(networkName);
        }
    }

    if (uniqueNetworkNames.size() <= 1)
    {
        return;
    }

    QString message;

    message += "The loaded ITF files appear to belong to different networks.\n\n";
    message += "File to network mapping:\n";

    for (const QString& line : fileNetworkLines)
    {
        message += line + "\n";
    }

    message += "\nYou can continue, but comparisons between different networks may not be meaningful.";

    QMessageBox::warning(
        this,
        "Different Networks Detected",
        message);
}

void MainWindow::dumpDockLayout(const QString& label) const
{
    qDebug().noquote()
        << "\n========== DOCK LAYOUT DUMP:"
        << label
        << "==========";

    qDebug().noquote()
        << "MainWindow geometry:"
        << geometry()
        << "size:"
        << size();

	qDebug().noquote()
	    << "MainWindow minimumSize:" << minimumSize()
	    << "minimumSizeHint:" << minimumSizeHint()
	    << "maximumSize:" << maximumSize()
	    << "isMaximized:" << isMaximized();

    if (centralWidget())
    {
        qDebug().noquote()
            << "Central widget geometry:"
            << centralWidget()->geometry()
            << "size:"
            << centralWidget()->size()
            << "min:"
            << centralWidget()->minimumSize()
            << "max:"
            << centralWidget()->maximumSize();
    }

    if (mFileSelectionDock)
    {
        qDebug().noquote()
            << "FilesDock"
            << "visible:" << mFileSelectionDock->isVisible()
            << "floating:" << mFileSelectionDock->isFloating()
            << "area:" << dockWidgetArea(mFileSelectionDock)
            << "geometry:" << mFileSelectionDock->geometry()
            << "size:" << mFileSelectionDock->size()
            << "min:" << mFileSelectionDock->minimumSize()
            << "max:" << mFileSelectionDock->maximumSize();

        if (mFileSelectionDock->widget())
        {
            qDebug().noquote()
                << "  FilesDock widget geometry:"
                << mFileSelectionDock->widget()->geometry()
                << "size:"
                << mFileSelectionDock->widget()->size();
        }
    }

    for (int i = 0; i < mPlotDocks.size(); ++i)
    {
        QDockWidget* dock =
            mPlotDocks[i];

        if (!dock)
        {
            qDebug().noquote()
                << "PlotDock" << i + 1 << ": null";
            continue;
        }

        qDebug().noquote()
            << "PlotDock" << i + 1
            << "objectName:" << dock->objectName()
            << "visible:" << dock->isVisible()
            << "floating:" << dock->isFloating()
            << "area:" << dockWidgetArea(dock)
            << "geometry:" << dock->geometry()
            << "size:" << dock->size()
            << "min:" << dock->minimumSize()
            << "max:" << dock->maximumSize();

        if (dock->widget())
        {
            qDebug().noquote()
                << "  PlotDock" << i + 1
                << "widget geometry:"
                << dock->widget()->geometry()
                << "size:"
                << dock->widget()->size()
                << "min:"
                << dock->widget()->minimumSize()
                << "max:"
                << dock->widget()->maximumSize();
        }
		if (i < mPlotBrowserWidgets.size() &&
		    mPlotBrowserWidgets[i])
		{
		    mPlotBrowserWidgets[i]->dumpLayoutDebug(label,
		                                            i + 1);
		}
    }

    qDebug().noquote()
        << "========== END DOCK LAYOUT DUMP ==========\n";
}

