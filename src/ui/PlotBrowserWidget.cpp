#include "PlotBrowserWidget.h"

#include "../model/Study.h"

#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <QPainter>
#include <QPixmap>

#include <QPushButton>

#include <QDebug>
#include <QLayout>
#include <QPoint>

//right click
#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>

namespace
{
    const QSize ContextExportPlotSize(1600, 900);

    QString safePlotExportFileName(const QString& text)
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

PlotBrowserWidget::PlotBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
	setMinimumSize(0, 0);
	setSizePolicy(QSizePolicy::Ignored,
				  QSizePolicy::Ignored);

    mStudyTypeCombo = new QComboBox(this);
    mComponentCombo = new QComboBox(this);
    mSignalCombo = new QComboBox(this);

	const int comboMinimumWidth = 240;
	
	mStudyTypeCombo->setMinimumWidth(comboMinimumWidth);
	mComponentCombo->setMinimumWidth(comboMinimumWidth);
	mSignalCombo->setMinimumWidth(comboMinimumWidth);


	mPlotTypeCombo = new QComboBox(this);
	
	mPlotTypeCombo->addItem("Line",
							static_cast<int>(PlotType::Line));
	
	mPlotTypeCombo->addItem("Dashed Line",
							static_cast<int>(PlotType::DashedLine));
	
	mPlotTypeCombo->addItem("Dotted Line",
							static_cast<int>(PlotType::DottedLine));
	
	mPlotTypeCombo->addItem("Dash-Dot Line",
							static_cast<int>(PlotType::DashDotLine));
	
	mPlotTypeCombo->addItem("Bar",
							static_cast<int>(PlotType::Bar));


    mSummaryLabel = new QLabel(this);
    mSummaryLabel->setWordWrap(true);
    mSummaryLabel->setText("No signal selected.");
	mSummaryLabel->setMaximumHeight(90);
	mSummaryLabel->setSizePolicy(QSizePolicy::Expanding,
                             QSizePolicy::Maximum);

    mPlotWidget = new PlotWidget(this);
    mPlotWidget->setSizePolicy(QSizePolicy::Expanding,
                               QSizePolicy::Expanding);

	mPlotWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	
	connect(mPlotWidget,
			&QWidget::customContextMenuRequested,
			this,
			&PlotBrowserWidget::showPlotContextMenu);

	connect(mPlotWidget,
			&PlotWidget::xRangeChanged,
			this,
			[this](double minX,
				   double maxX,
				   bool hasCustomRange)
			{
				emit plotXRangeChanged(this,
									   minX,
									   maxX,
									   hasCustomRange);
			});
			

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
	mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    //
	// Left control panel.
	//
	mControlPanel = new QFrame(this);
	mControlPanel->setFrameShape(QFrame::StyledPanel);
	mControlPanel->setMinimumWidth(360);
	mControlPanel->setMaximumWidth(440);

	QVBoxLayout* leftLayout = new QVBoxLayout(mControlPanel);
	leftLayout->setContentsMargins(10, 10, 10, 10);
	leftLayout->setSpacing(10);

    //
    // Selection card.
    //
    QFrame* selectionFrame = new QFrame(mControlPanel);
    selectionFrame->setFrameShape(QFrame::StyledPanel);

    QVBoxLayout* selectionLayout =
        new QVBoxLayout(selectionFrame);

    selectionLayout->setContentsMargins(10, 10, 10, 10);
    selectionLayout->setSpacing(8);

    QLabel* selectionTitleLabel =
        new QLabel("Plot Selection", selectionFrame);

    QFont selectionTitleFont =
        selectionTitleLabel->font();

    selectionTitleFont.setBold(true);
    selectionTitleLabel->setFont(selectionTitleFont);

    QFormLayout* selectionFormLayout =
        new QFormLayout;

    selectionFormLayout->setLabelAlignment(Qt::AlignLeft);
    selectionFormLayout->setFormAlignment(Qt::AlignTop);
    selectionFormLayout->setHorizontalSpacing(12);
    selectionFormLayout->setVerticalSpacing(8);
    selectionFormLayout->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);

    selectionFormLayout->addRow("Study Type", mStudyTypeCombo);
    selectionFormLayout->addRow("Component", mComponentCombo);
    selectionFormLayout->addRow("Signal", mSignalCombo);
    selectionFormLayout->addRow("Plot Type", mPlotTypeCombo);

    selectionLayout->addWidget(selectionTitleLabel);
    selectionLayout->addLayout(selectionFormLayout);

    leftLayout->addWidget(selectionFrame);

    //
    // Plot status card.
    //
    QFrame* statusFrame = new QFrame(mControlPanel);
    statusFrame->setFrameShape(QFrame::StyledPanel);

    QVBoxLayout* statusLayout =
        new QVBoxLayout(statusFrame);

    statusLayout->setContentsMargins(10, 10, 10, 10);
    statusLayout->setSpacing(8);

    QLabel* statusTitleLabel =
        new QLabel("Plot Status", statusFrame);

    QFont statusTitleFont =
        statusTitleLabel->font();

    statusTitleFont.setBold(true);
    statusTitleLabel->setFont(statusTitleFont);

    statusLayout->addWidget(statusTitleLabel);
    statusLayout->addWidget(mSummaryLabel);

    leftLayout->addWidget(statusFrame);
	leftLayout->addStretch();

	//
	// Right plot panel.
	// The toggle button remains visible even when the left selection panel is hidden.
	//
	QWidget* rightPanel = new QWidget(this);

	QVBoxLayout* rightLayout =
	    new QVBoxLayout(rightPanel);

	rightLayout->setContentsMargins(0, 0, 0, 0);
	rightLayout->setSpacing(4);

	QHBoxLayout* toggleLayout =
	    new QHBoxLayout;

	toggleLayout->setContentsMargins(0, 0, 0, 0);
	toggleLayout->addStretch();

	mToggleControlsButton =
	    new QPushButton("Hide Plot Selection", rightPanel);

	toggleLayout->addWidget(mToggleControlsButton);

	rightLayout->addLayout(toggleLayout);
	rightLayout->addWidget(mPlotWidget, 1);

	mainLayout->addWidget(mControlPanel);
	mainLayout->addWidget(rightPanel, 1);

	connect(mToggleControlsButton,
	        &QPushButton::clicked,
	        this,
	        &PlotBrowserWidget::togglePlotSelectionPanel);

    connect(mStudyTypeCombo,
            &QComboBox::currentIndexChanged,
            this,
            &PlotBrowserWidget::onStudyTypeChanged);

    connect(mComponentCombo,
            &QComboBox::currentIndexChanged,
            this,
            &PlotBrowserWidget::onComponentChanged);

    connect(mSignalCombo,
            &QComboBox::currentIndexChanged,
            this,
            &PlotBrowserWidget::onSignalChanged);

    connect(mPlotTypeCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &PlotBrowserWidget::onPlotTypeChanged);
}

void PlotBrowserWidget::applyExternalXRange(double minX,
                                            double maxX,
                                            bool hasCustomRange)
{
    if (!mPlotWidget)
    {
        return;
    }

    mPlotWidget->applyExternalXRange(minX,
                                     maxX,
                                     hasCustomRange);
}

void PlotBrowserWidget::showPlotContextMenu(const QPoint& position)
{
    if (!mPlotWidget)
    {
        return;
    }

    QMenu menu(this);

    QAction* exportAction =
        menu.addAction("Export Plot as PNG...");

    exportAction->setEnabled(!mExportSeriesList.isEmpty());

    menu.addSeparator();

	QAction* showMajorGridAction =
		menu.addAction("Show Major Grid");
	
	showMajorGridAction->setCheckable(true);
	showMajorGridAction->setChecked(mPlotWidget->showMajorGrid());
	
	QAction* showMinorGridAction =
		menu.addAction("Show Minor Grid");
	
	showMinorGridAction->setCheckable(true);
	showMinorGridAction->setChecked(mPlotWidget->showMinorGrid());


    QAction* selectedAction =
        menu.exec(mPlotWidget->mapToGlobal(position));

    if (selectedAction == exportAction)
    {
        exportCurrentPlotAsPng();
        return;
    }

	if (selectedAction == showMajorGridAction)
	{
		mPlotWidget->setShowMajorGrid(
			showMajorGridAction->isChecked());
	
		return;
	}
	
	if (selectedAction == showMinorGridAction)
	{
		mPlotWidget->setShowMinorGrid(
			showMinorGridAction->isChecked());
	
		return;
	}

}

void PlotBrowserWidget::exportCurrentPlotAsPng()
{
    if (mExportSeriesList.isEmpty())
    {
        QMessageBox::information(
            this,
            "No plot",
            "There is no plot data to export.");

        return;
    }

    QString defaultFileName =
        safePlotExportFileName(exportPlotTitle());

    if (!defaultFileName.endsWith(".png",
                                  Qt::CaseInsensitive))
    {
        defaultFileName += ".png";
    }

    QString filePath =
        QFileDialog::getSaveFileName(
            this,
            "Export Plot as PNG",
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

    const QPixmap pixmap =
        exportPlotPixmap(ContextExportPlotSize);

    if (pixmap.isNull())
    {
        QMessageBox::critical(
            this,
            "Export failed",
            "Could not render the plot.");

        return;
    }

    if (!pixmap.save(filePath, "PNG"))
    {
        QMessageBox::critical(
            this,
            "Export failed",
            "Could not save the plot as PNG.");

        return;
    }
}

void PlotBrowserWidget::setStudies(
    const QVector<const Study*>& studies)
{
    mStudies = studies;

    populateStudyTypes();
}

void PlotBrowserWidget::setFilePlotSettings(
    const QVector<FilePlotSettings>& settings)
{
    mFilePlotSettings = settings;

    updatePlot();
}

void PlotBrowserWidget::populateStudyTypes()
{
    /*
     * Capture current selections before clearing any combo box.
     * This preserves each plot window selection when files are added/removed.
     */
    const QString previouslySelectedStudyType =
        currentRawStudyType();

    const QVariant previouslySelectedComponent =
        mComponentCombo
            ? mComponentCombo->currentData()
            : QVariant();

    const QString previouslySelectedSignal =
        currentRawSignalName();

    mStudyTypeCombo->blockSignals(true);
    mComponentCombo->blockSignals(true);
    mSignalCombo->blockSignals(true);

    mStudyTypeCombo->clear();
    mComponentCombo->clear();
    mSignalCombo->clear();

    mSummaryLabel->clear();

    if (mPlotWidget)
    {
        mPlotWidget->clear();
    }

    const Study* study =
        referenceStudy();

    if (!study)
    {
        mStudyTypeCombo->blockSignals(false);
        mComponentCombo->blockSignals(false);
        mSignalCombo->blockSignals(false);
        return;
    }

    const QStringList studyTypes =
        study->getStudyTypeNames();

    for (const QString& rawStudyType : studyTypes)
    {
        const QString displayStudyType =
            PlotDisplayMapper::displayStudyTypeName(
                rawStudyType,
                mNameDisplayMode);

        mStudyTypeCombo->addItem(displayStudyType,
                                 rawStudyType);
    }

    const int previousStudyTypeIndex =
        mStudyTypeCombo->findData(previouslySelectedStudyType);

    if (previousStudyTypeIndex >= 0)
    {
        mStudyTypeCombo->setCurrentIndex(previousStudyTypeIndex);
    }
    else if (mStudyTypeCombo->count() > 0)
    {
        mStudyTypeCombo->setCurrentIndex(0);
    }

    mStudyTypeCombo->blockSignals(false);
    mComponentCombo->blockSignals(false);
    mSignalCombo->blockSignals(false);

    populateComponents();

    /*
     * populateComponents() and populateSignals() already try to preserve
     * their own current values, but during a full refresh the combo boxes
     * were cleared above. Restore component/signal explicitly here.
     */
    const int previousComponentIndex =
        mComponentCombo->findData(previouslySelectedComponent);

    if (previousComponentIndex >= 0)
    {
        mComponentCombo->setCurrentIndex(previousComponentIndex);
    }

    populateSignals();

    const int previousSignalIndex =
        mSignalCombo->findData(previouslySelectedSignal);

    if (previousSignalIndex >= 0)
    {
        mSignalCombo->setCurrentIndex(previousSignalIndex);
    }

    updatePlot();
}

void PlotBrowserWidget::populateComponents()
{
    const QVariant previouslySelectedComponent =
        mComponentCombo->currentData();

    mComponentCombo->blockSignals(true);
    mComponentCombo->clear();

    const Study* study =
        referenceStudy();

    if (!study)
    {
        mComponentCombo->blockSignals(false);
        populateSignals();
        return;
    }

    const QString studyType =
    currentRawStudyType();

    const QString targetGroup =
        study->getTargetGroupForStudyType(studyType);

    if (targetGroup.isEmpty())
    {
        mComponentCombo->blockSignals(false);
        populateSignals();
        return;
    }

    const QList<int> componentIds =
        study->getComponentIds(targetGroup);

    for (int componentId : componentIds)
    {
        const QString componentName =
            study->getComponentName(targetGroup,
                                    componentId);

        const QString displayText =
            QString("[%1] %2")
                .arg(componentId)
                .arg(componentName);

        mComponentCombo->addItem(displayText,
                                 componentId);
    }

    const int previousIndex =
        mComponentCombo->findData(previouslySelectedComponent);

    if (previousIndex >= 0)
    {
        mComponentCombo->setCurrentIndex(previousIndex);
    }
    else if (mComponentCombo->count() > 0)
    {
        mComponentCombo->setCurrentIndex(0);
    }

    mComponentCombo->blockSignals(false);

    populateSignals();
}

void PlotBrowserWidget::populateSignals()
{
    const QString previouslySelectedSignal =
    	currentRawSignalName();

    mSignalCombo->blockSignals(true);
    mSignalCombo->clear();

    const Study* study =
        referenceStudy();

    if (!study)
    {
        mSignalCombo->blockSignals(false);
        updatePlot();
        return;
    }

    const QString studyType =
    	currentRawStudyType();

    if (studyType.isEmpty())
    {
        mSignalCombo->blockSignals(false);
        updatePlot();
        return;
    }

    const QStringList signalNames =
        study->getSignalSchemaNames(studyType,
                                    false);

	for (const QString& rawSignalName : signalNames)
	{
		const QString displaySignalName =
			PlotDisplayMapper::displaySignalName(
				studyType,
				rawSignalName,
				mNameDisplayMode);
	
		mSignalCombo->addItem(displaySignalName,
							  rawSignalName);
	}

    const int previousSignalIndex =
    	mSignalCombo->findData(previouslySelectedSignal);

    if (previousSignalIndex >= 0)
    {
        mSignalCombo->setCurrentIndex(previousSignalIndex);
    }
    else if (mSignalCombo->count() > 0)
    {
        mSignalCombo->setCurrentIndex(0);
    }

    mSignalCombo->blockSignals(false);

    updatePlot();
}

void PlotBrowserWidget::updatePlot()
{
    const Study* reference =
        referenceStudy();

    if (!reference)
    {
        mSummaryLabel->setText("No study loaded.");
		clearExportCache();

        if (mPlotWidget)
        {
            mPlotWidget->clear();
        }

        return;
    }

	const QString studyType =
		currentRawStudyType();

    const QString targetGroup =
        reference->getTargetGroupForStudyType(studyType);

	const QString signalName =
		currentRawSignalName();

    const int componentId =
        mComponentCombo->currentData().toInt();

    if (studyType.isEmpty() ||
        targetGroup.isEmpty() ||
        signalName.isEmpty())
    {
        mSummaryLabel->setText("No signal selected.");
		clearExportCache();

        if (mPlotWidget)
        {
            mPlotWidget->clear();
        }

        return;
    }

    QVector<PlotSeries> seriesList;
    QStringList unavailableFiles;

    int enabledFileCount = 0;
    int plottedFileCount = 0;

    for (int i = 0; i < mStudies.size(); ++i)
    {
        if (i >= mFilePlotSettings.size())
        {
            continue;
        }

        const FilePlotSettings& settings =
            mFilePlotSettings[i];

        if (!settings.mEnabled)
        {
            continue;
        }

        ++enabledFileCount;

        const Study* study =
            mStudies[i];

        if (!study)
        {
            continue;
        }

        const QString fileTargetGroup =
            study->getTargetGroupForStudyType(studyType);

        const QVector<double> timeValues =
            study->getTimeValues();

        const QVector<double> signalValues =
            study->getSignalValues(fileTargetGroup,
                                   componentId,
                                   signalName);

        const QString fileName =
            studyDisplayName(study);

        if (!timeValues.isEmpty() &&
            !signalValues.isEmpty() &&
            timeValues.size() == signalValues.size())
        {
            PlotSeries series;
            series.mName = fileName;
            series.mXValues = timeValues;
            series.mYValues = signalValues;
            series.mColor = settings.mColor;
            series.mLineThickness = settings.mLineThickness;

            seriesList.append(series);

            ++plottedFileCount;
        }
        else
        {
            unavailableFiles.append(fileName);
        }
    }

    if (enabledFileCount == 0)
    {
        mSummaryLabel->setText("No files selected for plotting.");
		clearExportCache();

        if (mPlotWidget)
        {
            mPlotWidget->clear();
        }

        return;
    }

    QString summary;

    summary += QString("Plotted: %1 / %2 selected file(s)\n")
                   .arg(plottedFileCount)
                   .arg(enabledFileCount);

    if (!unavailableFiles.isEmpty())
    {
        summary += "\nUnavailable in:\n";

        for (const QString& fileName : unavailableFiles)
        {
            summary += QString("  - %1\n")
                           .arg(fileName);
        }
    }

    mSummaryLabel->setText(summary);

	const QString displayStudyType =
	    mStudyTypeCombo
	        ? mStudyTypeCombo->currentText()
	        : studyType;

	const QString displaySignalName =
	    mSignalCombo
	        ? mSignalCombo->currentText()
	        : signalName;

	const QString title =
		QString("%1 [%2] - %3")
			.arg(displayStudyType)
			.arg(componentId)
			.arg(displaySignalName);


	const QString xAxisLabel =
		"Time (s)";
	
	const QString yAxisLabel =
		signalName;
	
	mExportSeriesList =
		seriesList;
	
	mExportTitle =
		title;
	
	mExportXAxisLabel =
		xAxisLabel;
	
	mExportYAxisLabel =
		yAxisLabel;
	
	mExportPlotType =
		currentPlotType();
	
	if (mPlotWidget)
	{
		mPlotWidget->setPlotType(mExportPlotType);
	
		mPlotWidget->setSeries(mExportSeriesList,
							   mExportTitle,
							   mExportXAxisLabel,
							   mExportYAxisLabel);
	}

}

const Study* PlotBrowserWidget::referenceStudy() const
{
    if (mStudies.isEmpty())
    {
        return nullptr;
    }

    return mStudies.first();
}

QString PlotBrowserWidget::studyDisplayName(
    const Study* study) const
{
    if (!study)
    {
        return QString();
    }

    const QFileInfo fileInfo(study->getFileName());

    /*
     * Show file name without .itf extension in plot legend / hover details.
     *
     * Example:
     *   first.itf -> first
     *   DynamicPluginProf.itf -> DynamicPluginProf
     */
    const QString nameWithoutExtension =
        fileInfo.completeBaseName();

    if (!nameWithoutExtension.isEmpty())
    {
        return nameWithoutExtension;
    }

    if (!fileInfo.fileName().isEmpty())
    {
        return fileInfo.fileName();
    }

    return study->getFileName();
}

PlotType PlotBrowserWidget::currentPlotType() const
{
    if (!mPlotTypeCombo)
    {
        return PlotType::Line;
    }

    return static_cast<PlotType>(
        mPlotTypeCombo->currentData().toInt());
}

QString PlotBrowserWidget::currentRawStudyType() const
{
    if (!mStudyTypeCombo)
    {
        return QString();
    }

    const QString rawStudyType =
        mStudyTypeCombo->currentData().toString();

    if (!rawStudyType.isEmpty())
    {
        return rawStudyType;
    }

    return mStudyTypeCombo->currentText();
}

QString PlotBrowserWidget::currentRawSignalName() const
{
    if (!mSignalCombo)
    {
        return QString();
    }

    const QString rawSignalName =
        mSignalCombo->currentData().toString();

    if (!rawSignalName.isEmpty())
    {
        return rawSignalName;
    }

    return mSignalCombo->currentText();
}

QPixmap PlotBrowserWidget::exportPlotPixmap(int targetWidth) const
{
    /*
     * Backward-compatible wrapper.
     * Route the old API through the fixed-size export path.
     */
    if (targetWidth <= 0)
    {
        targetWidth = 1600;
    }

    const int targetHeight =
        qMax(1,
             static_cast<int>(
                 static_cast<double>(targetWidth) * 9.0 / 16.0));

    return exportPlotPixmap(QSize(targetWidth,
                                  targetHeight));
}

void PlotBrowserWidget::onStudyTypeChanged()
{
    populateComponents();
}

void PlotBrowserWidget::onComponentChanged()
{
    populateSignals();
}

void PlotBrowserWidget::onSignalChanged()
{
    updatePlot();
}

void PlotBrowserWidget::onPlotTypeChanged()
{
    updatePlot();
}

void PlotBrowserWidget::togglePlotSelectionPanel()
{
    if (!mControlPanel)
    {
        return;
    }

    const bool shouldShow =
        !mControlPanel->isVisible();

    mControlPanel->setVisible(shouldShow);

    if (mToggleControlsButton)
    {
        mToggleControlsButton->setText(
            shouldShow
                ? "Hide Plot Selection"
                : "Show Plot Selection");
    }

    if (mPlotWidget)
    {
        mPlotWidget->updateGeometry();
        mPlotWidget->update();
    }

    updateGeometry();
}

void PlotBrowserWidget::setNameDisplayMode(NameDisplayMode mode)
{
    if (mNameDisplayMode == mode)
    {
        return;
    }

    mNameDisplayMode = mode;

    populateStudyTypes();
}

QSize PlotBrowserWidget::sizeHint() const
{
    /*
     * Preferred size when space is available.
     * This is not a hard constraint.
     */
    return QSize(760, 420);
}

QSize PlotBrowserWidget::minimumSizeHint() const
{
    /*
     * Important for QDockWidget:
     * Keep this small so four docked plot windows can fit in a 2 x 2 layout.
     *
     * The internal controls may visually compress if the dock is very small,
     * but this prevents QMainWindow from growing beyond the screen.
     */
    return QSize(320, 180);
}

void PlotBrowserWidget::clearExportCache()
{
    mExportSeriesList.clear();
    mExportTitle.clear();
    mExportXAxisLabel.clear();
    mExportYAxisLabel.clear();
    mExportPlotType = PlotType::Line;
}

QPixmap PlotBrowserWidget::exportPlotPixmap(
    const QSize& exportSize) const
{
    QSize finalExportSize =
        exportSize;

    if (!finalExportSize.isValid() ||
        finalExportSize.width() <= 0 ||
        finalExportSize.height() <= 0)
    {
        finalExportSize = QSize(1600, 900);
    }

    /*
     * Important:
     * Do not export mPlotWidget directly.
     *
     * mPlotWidget is affected by the current UI/dock size.
     * Instead, create a temporary off-screen PlotWidget with fixed export
     * dimensions and render that.
     */
    PlotWidget exportWidget;

    exportWidget.resize(finalExportSize);
    exportWidget.setMinimumSize(finalExportSize);
    exportWidget.setMaximumSize(finalExportSize);

    exportWidget.setPlotType(mExportPlotType);

	if (mPlotWidget)
	{
		exportWidget.setShowMajorGrid(mPlotWidget->showMajorGrid());
		exportWidget.setShowMinorGrid(mPlotWidget->showMinorGrid());
	}

    exportWidget.setSeries(mExportSeriesList,
                           mExportTitle,
                           mExportXAxisLabel,
                           mExportYAxisLabel);

    QPixmap pixmap(finalExportSize);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    exportWidget.render(&painter);

    return pixmap;
}

QString PlotBrowserWidget::exportPlotTitle() const
{
    if (!mExportTitle.isEmpty())
    {
        return mExportTitle;
    }

    return "Plot";
}

void PlotBrowserWidget::dumpLayoutDebug(const QString& label,
                                        int plotIndex) const
{
    qDebug().noquote()
        << "\n----- PlotBrowserWidget Debug:"
        << label
        << "Plot"
        << plotIndex
        << "-----";

    qDebug().noquote()
        << "PlotBrowserWidget"
        << "geometry:" << geometry()
        << "size:" << size()
        << "minimumSize:" << minimumSize()
        << "maximumSize:" << maximumSize()
        << "sizeHint:" << sizeHint()
        << "minimumSizeHint:" << minimumSizeHint();

    if (layout())
    {
        qDebug().noquote()
            << "Main layout"
            << "sizeConstraint:" << layout()->sizeConstraint()
            << "contentsMargins:" << layout()->contentsMargins()
            << "spacing:" << layout()->spacing();
    }

    if (mControlPanel)
    {
        qDebug().noquote()
            << "mControlPanel"
            << "visible:" << mControlPanel->isVisible()
            << "geometry:" << mControlPanel->geometry()
            << "size:" << mControlPanel->size()
            << "minimumSize:" << mControlPanel->minimumSize()
            << "maximumSize:" << mControlPanel->maximumSize()
            << "sizeHint:" << mControlPanel->sizeHint()
            << "minimumSizeHint:" << mControlPanel->minimumSizeHint();

        if (mControlPanel->layout())
        {
            qDebug().noquote()
                << "mControlPanel layout"
                << "sizeConstraint:" << mControlPanel->layout()->sizeConstraint()
                << "contentsMargins:" << mControlPanel->layout()->contentsMargins()
                << "spacing:" << mControlPanel->layout()->spacing();
        }
    }

    if (mToggleControlsButton)
    {
        qDebug().noquote()
            << "mToggleControlsButton"
            << "visible:" << mToggleControlsButton->isVisible()
            << "geometry:" << mToggleControlsButton->geometry()
            << "size:" << mToggleControlsButton->size()
            << "minimumSize:" << mToggleControlsButton->minimumSize()
            << "sizeHint:" << mToggleControlsButton->sizeHint()
            << "minimumSizeHint:" << mToggleControlsButton->minimumSizeHint();
    }

    if (mPlotWidget)
    {
        qDebug().noquote()
            << "mPlotWidget"
            << "geometry:" << mPlotWidget->geometry()
            << "size:" << mPlotWidget->size()
            << "minimumSize:" << mPlotWidget->minimumSize()
            << "maximumSize:" << mPlotWidget->maximumSize()
            << "sizeHint:" << mPlotWidget->sizeHint()
            << "minimumSizeHint:" << mPlotWidget->minimumSizeHint();

        QWidget* rightPanel =
            mPlotWidget->parentWidget();

        if (rightPanel)
        {
            qDebug().noquote()
                << "rightPanel"
                << "geometry:" << rightPanel->geometry()
                << "size:" << rightPanel->size()
                << "minimumSize:" << rightPanel->minimumSize()
                << "maximumSize:" << rightPanel->maximumSize()
                << "sizeHint:" << rightPanel->sizeHint()
                << "minimumSizeHint:" << rightPanel->minimumSizeHint();

            if (rightPanel->layout())
            {
                qDebug().noquote()
                    << "rightPanel layout"
                    << "sizeConstraint:" << rightPanel->layout()->sizeConstraint()
                    << "contentsMargins:" << rightPanel->layout()->contentsMargins()
                    << "spacing:" << rightPanel->layout()->spacing();
            }
        }
    }

    if (mStudyTypeCombo)
    {
        qDebug().noquote()
            << "mStudyTypeCombo"
            << "text:" << mStudyTypeCombo->currentText()
            << "geometry:" << mStudyTypeCombo->geometry()
            << "minimumSize:" << mStudyTypeCombo->minimumSize()
            << "sizeHint:" << mStudyTypeCombo->sizeHint()
            << "minimumSizeHint:" << mStudyTypeCombo->minimumSizeHint();
    }

    if (mComponentCombo)
    {
        qDebug().noquote()
            << "mComponentCombo"
            << "text:" << mComponentCombo->currentText()
            << "geometry:" << mComponentCombo->geometry()
            << "minimumSize:" << mComponentCombo->minimumSize()
            << "sizeHint:" << mComponentCombo->sizeHint()
            << "minimumSizeHint:" << mComponentCombo->minimumSizeHint();
    }

    if (mSignalCombo)
    {
        qDebug().noquote()
            << "mSignalCombo"
            << "text:" << mSignalCombo->currentText()
            << "geometry:" << mSignalCombo->geometry()
            << "minimumSize:" << mSignalCombo->minimumSize()
            << "sizeHint:" << mSignalCombo->sizeHint()
            << "minimumSizeHint:" << mSignalCombo->minimumSizeHint();
    }

    if (mSummaryLabel)
    {
        qDebug().noquote()
            << "mSummaryLabel"
            << "text:" << mSummaryLabel->text().replace("\n", " | ")
            << "geometry:" << mSummaryLabel->geometry()
            << "minimumSize:" << mSummaryLabel->minimumSize()
            << "maximumSize:" << mSummaryLabel->maximumSize()
            << "sizeHint:" << mSummaryLabel->sizeHint()
            << "minimumSizeHint:" << mSummaryLabel->minimumSizeHint();
    }

    qDebug().noquote()
        << "----- End PlotBrowserWidget Debug -----\n";
}

