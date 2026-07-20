#include "StudyBrowserWidget.h"

#include "../model/Study.h"
#include "PlotWidget.h"

#include <QComboBox>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFormLayout>
#include <QFont>
#include <QSizePolicy>
#include <QCheckBox>
#include <QCheckBox>
#include <QSizePolicy>

namespace
{
    struct ColorOption
    {
        QString mName;
        QColor mColor;
    };

    QVector<ColorOption> comparisonColorOptions()
    {
        return
        {
            { "Blue", QColor(0, 90, 180) },
            { "Red", QColor(190, 60, 50) },
            { "Green", QColor(40, 140, 80) },
            { "Purple", QColor(120, 80, 170) },
            { "Orange", QColor(200, 130, 30) },
            { "Cyan", QColor(0, 150, 170) },
            { "Magenta", QColor(180, 70, 140) },
            { "Brown", QColor(130, 90, 50) },
            { "Dark Gray", QColor(80, 80, 80) },
            { "Black", QColor(0, 0, 0) }
        };
    }
}

StudyBrowserWidget::StudyBrowserWidget(QWidget* parent)
    : QWidget(parent)
{

//file info frame
#if 1
	mFileInfoFrame = new QFrame(this);
	mFileInfoFrame->setFrameShape(QFrame::StyledPanel);
	mFileInfoFrame->setFrameShadow(QFrame::Raised);

	QGridLayout* fileInfoLayout = new QGridLayout(mFileInfoFrame);
	fileInfoLayout->setContentsMargins(12, 10, 12, 10);
	fileInfoLayout->setHorizontalSpacing(16);
	fileInfoLayout->setVerticalSpacing(6);

	QLabel* titleLabel = new QLabel("Current Study", mFileInfoFrame);

	QFont titleFont = titleLabel->font();
	titleFont.setBold(true);
	titleFont.setPointSize(titleFont.pointSize() + 1);
	titleLabel->setFont(titleFont);

	QLabel* fileCountLabel = new QLabel("Files", mFileInfoFrame);
	QLabel* loadedFilesLabel = new QLabel("Loaded files", mFileInfoFrame);
	
	mFileCountValueLabel = new QLabel("0 / 5", mFileInfoFrame);
	
	mLoadedFilesValueLabel = new QLabel("No file loaded", mFileInfoFrame);
	mLoadedFilesValueLabel->setWordWrap(true);
	
	fileInfoLayout->addWidget(titleLabel, 0, 0, 1, 2);
	
	fileInfoLayout->addWidget(fileCountLabel, 1, 0);
	fileInfoLayout->addWidget(mFileCountValueLabel, 1, 1);
	
	fileInfoLayout->addWidget(loadedFilesLabel, 2, 0);
	fileInfoLayout->addWidget(mLoadedFilesValueLabel, 2, 1);
	
	fileInfoLayout->setColumnStretch(1, 1);

#endif

   	mStudyTypeCombo = new QComboBox(this);
	mComponentCombo = new QComboBox(this);
	mSignalCombo = new QComboBox(this);
	
	mPlotTypeCombo = new QComboBox(this);
	mPlotTypeCombo->addItem("Line", static_cast<int>(PlotType::Line));
	mPlotTypeCombo->addItem("Bar", static_cast<int>(PlotType::Bar));
	
	mSummaryLabel = new QLabel(this);
	mPlotWidget = new PlotWidget(this);

    mSummaryLabel->setWordWrap(true);

	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(8);
	
	QFrame* leftPanel = new QFrame(this);
	leftPanel->setFrameShape(QFrame::StyledPanel);
	leftPanel->setMinimumWidth(330);
	leftPanel->setMaximumWidth(390);
	
	QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
	leftLayout->setContentsMargins(10, 10, 10, 10);
	leftLayout->setSpacing(10);
	
	leftLayout->addWidget(mFileInfoFrame);
	
	//
	// Selection card
	//
	QFrame* selectionFrame = new QFrame(leftPanel);
	selectionFrame->setFrameShape(QFrame::StyledPanel);
	
	QVBoxLayout* selectionLayout = new QVBoxLayout(selectionFrame);
	selectionLayout->setContentsMargins(10, 10, 10, 10);
	selectionLayout->setSpacing(8);
	
	QLabel* selectionTitleLabel = new QLabel("Selection", selectionFrame);
	
	QFont selectionTitleFont = selectionTitleLabel->font();
	selectionTitleFont.setBold(true);
	selectionTitleLabel->setFont(selectionTitleFont);
	
	QFormLayout* selectionFormLayout = new QFormLayout;
	selectionFormLayout->setLabelAlignment(Qt::AlignLeft);
	selectionFormLayout->setFormAlignment(Qt::AlignTop);
	selectionFormLayout->setHorizontalSpacing(12);
	selectionFormLayout->setVerticalSpacing(8);
	selectionFormLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	
	selectionFormLayout->addRow("Study Type", mStudyTypeCombo);
	selectionFormLayout->addRow("Component", mComponentCombo);
	selectionFormLayout->addRow("Signal", mSignalCombo);
	selectionFormLayout->addRow("Plot Type", mPlotTypeCombo);
	
	selectionLayout->addWidget(selectionTitleLabel);
	selectionLayout->addLayout(selectionFormLayout);
	
	leftLayout->addWidget(selectionFrame);

//chekbox for file selection
	mFileSelectionFrame = new QFrame(leftPanel);
	mFileSelectionFrame->setFrameShape(QFrame::StyledPanel);
	
	mFileSelectionLayout = new QVBoxLayout(mFileSelectionFrame);
	mFileSelectionLayout->setContentsMargins(10, 10, 10, 10);
	mFileSelectionLayout->setSpacing(6);
	
	QLabel* fileSelectionTitle = new QLabel("Files to Plot", mFileSelectionFrame);
	
	QFont fileSelectionTitleFont = fileSelectionTitle->font();
	fileSelectionTitleFont.setBold(true);
	fileSelectionTitle->setFont(fileSelectionTitleFont);
	
	mFileSelectionLayout->addWidget(fileSelectionTitle);
	
	leftLayout->addWidget(mFileSelectionFrame);

	
	//
	// Plot status card
	//
	QFrame* statusFrame = new QFrame(leftPanel);
	statusFrame->setFrameShape(QFrame::StyledPanel);
	
	QVBoxLayout* statusLayout = new QVBoxLayout(statusFrame);
	statusLayout->setContentsMargins(10, 10, 10, 10);
	statusLayout->setSpacing(8);
	
	QLabel* statusTitleLabel = new QLabel("Plot Status", statusFrame);
	
	QFont statusTitleFont = statusTitleLabel->font();
	statusTitleFont.setBold(true);
	statusTitleLabel->setFont(statusTitleFont);
	
	mSummaryLabel->setWordWrap(true);
	mSummaryLabel->setText("No signal selected.");
	
	statusLayout->addWidget(statusTitleLabel);
	statusLayout->addWidget(mSummaryLabel);
	
	leftLayout->addWidget(statusFrame);
	leftLayout->addStretch();
	
	mPlotWidget->setSizePolicy(QSizePolicy::Expanding,
							   QSizePolicy::Expanding);
	
	mainLayout->addWidget(leftPanel);
	mainLayout->addWidget(mPlotWidget, 1);


    connect(mStudyTypeCombo,
	        &QComboBox::currentIndexChanged,
	        this,
	        &StudyBrowserWidget::onStudyTypeChanged);

	connect(mComponentCombo,
	        &QComboBox::currentIndexChanged,
	        this,
	        &StudyBrowserWidget::onComponentChanged);

	connect(mSignalCombo,
	        &QComboBox::currentIndexChanged,
	        this,
	        &StudyBrowserWidget::onSignalChanged);
	connect(mPlotTypeCombo,
	        QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this,
	        &StudyBrowserWidget::onPlotTypeChanged);
}

void StudyBrowserWidget::setStudy(const Study* study)
{
    QVector<const Study*> studies;

    if (study)
    {
        studies.append(study);
    }

    setStudies(studies);
}

void StudyBrowserWidget::setStudies(const Study* primaryStudy,
                                    const Study* compareStudy)
{
    QVector<const Study*> studies;

    if (primaryStudy)
    {
        studies.append(primaryStudy);
    }

    if (compareStudy)
    {
        studies.append(compareStudy);
    }

    setStudies(studies);
}

void StudyBrowserWidget::setStudies(const QVector<const Study*>& studies)
{
    mStudies = studies;

    updateFileInfo();
    rebuildFileSelection();
    populateStudyTypes();
}

void StudyBrowserWidget::populateStudyTypes()
{
    mStudyTypeCombo->clear();
    mComponentCombo->clear();
    mSignalCombo->clear();
    mSummaryLabel->clear();

    if (mPlotWidget)
    {
        mPlotWidget->clear();
    }

    const Study* study = referenceStudy();

    if (!study)
    {
        return;
    }

    const QStringList studyTypes =
        study->getStudyTypeNames();

    for (const QString& studyType : studyTypes)
    {
        mStudyTypeCombo->addItem(studyType);
    }

    populateComponents();
}

void StudyBrowserWidget::populateComponents()
{
    mComponentCombo->clear();
    mSignalCombo->clear();
    mSummaryLabel->clear();

    if (mPlotWidget)
    {
        mPlotWidget->clear();
    }

    const Study* study = referenceStudy();

    if (!study)
    {
        return;
    }

    const QString studyType =
        mStudyTypeCombo->currentText();

    const QString targetGroup =
        study->getTargetGroupForStudyType(studyType);

    if (targetGroup.isEmpty())
    {
        return;
    }

    const QList<int> componentIds =
        study->getComponentIds(targetGroup);

    for (int componentId : componentIds)
    {
        const QString componentName =
            study->getComponentName(targetGroup, componentId);

        const QString displayText =
            QString("[%1] %2")
                .arg(componentId)
                .arg(componentName);

        mComponentCombo->addItem(displayText, componentId);
    }

    populateSignals();
}

void StudyBrowserWidget::populateSignals()
{
    const QString previouslySelectedSignal =
        mSignalCombo->currentText();

    mSignalCombo->blockSignals(true);
    mSignalCombo->clear();

    const Study* study = referenceStudy();

    if (!study)
    {
        mSignalCombo->blockSignals(false);

        mSummaryLabel->clear();

        if (mPlotWidget)
        {
            mPlotWidget->clear();
        }

        return;
    }

    const QString studyType =
        mStudyTypeCombo->currentText();

    if (studyType.isEmpty())
    {
        mSignalCombo->blockSignals(false);

        mSummaryLabel->clear();

        if (mPlotWidget)
        {
            mPlotWidget->clear();
        }

        return;
    }

    const QStringList signalNames =
        study->getSignalSchemaNames(studyType, false);

    for (const QString& signalName : signalNames)
    {
        mSignalCombo->addItem(signalName);
    }

    const int previousSignalIndex =
        mSignalCombo->findText(previouslySelectedSignal);

    if (previousSignalIndex >= 0)
    {
        mSignalCombo->setCurrentIndex(previousSignalIndex);
    }
    else if (mSignalCombo->count() > 0)
    {
        mSignalCombo->setCurrentIndex(0);
    }

    mSignalCombo->blockSignals(false);

    updateSignalSummary();
}

void StudyBrowserWidget::updateSignalSummary()
{
    const Study* reference = referenceStudy();

    if (!reference)
    {
        return;
    }

    const QString studyType =
        mStudyTypeCombo->currentText();

    const QString referenceTargetGroup =
        reference->getTargetGroupForStudyType(studyType);

    const int componentId =
        mComponentCombo->currentData().toInt();

    const QString componentText =
        mComponentCombo->currentText();

    const QString signalName =
        mSignalCombo->currentText();

    if (studyType.isEmpty() ||
        referenceTargetGroup.isEmpty() ||
        signalName.isEmpty())
    {
        mSummaryLabel->clear();

        if (mPlotWidget)
        {
            mPlotWidget->clear();
        }

        return;
    }

    QVector<PlotSeries> seriesList;

    int availableFileCount = 0;
    QStringList unavailableFiles;

	const QVector<int> selectedIndexes = selectedStudyIndexes();
	
	for (int selectedIndex : selectedIndexes)
    {
        if (selectedIndex < 0 || selectedIndex >= mStudies.size())
		    continue;

		const Study* study = mStudies[selectedIndex];

        if (!study)
        {
            continue;
        }

        const QString targetGroup =
            study->getTargetGroupForStudyType(studyType);

        const QVector<double> timeValues =
            study->getTimeValues();

        const QVector<double> signalValues =
            study->getSignalValues(targetGroup,
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
			series.mColor = colorForFileIndex(selectedIndex);
			series.mLineThickness = thicknessForFileIndex(selectedIndex);

			seriesList.append(series);
            availableFileCount++;
        }
        else
        {
            unavailableFiles.append(fileName);
        }
    }

	QString summary;
	
	summary += QString("Plotted: %1 / %2 file(s)\n")
				   .arg(availableFileCount)
				   .arg(selectedIndexes.size());

    if (!unavailableFiles.isEmpty())
    {
        summary += "\nUnavailable in:\n";

        for (const QString& fileName : unavailableFiles)
        {
            summary += QString("  - %1\n").arg(fileName);
        }
    }

	if (selectedIndexes.isEmpty())
	{
	    mSummaryLabel->setText("No files selected for plotting.");

	    if (mPlotWidget)
	        mPlotWidget->clear();

	    return;
	}

    mSummaryLabel->setText(summary);

    const QString title =
        QString("%1 [%2] - %3")
            .arg(studyType)
            .arg(componentId)
            .arg(signalName);

    if (mPlotWidget)
    {
        mPlotWidget->setPlotType(currentPlotType());

        mPlotWidget->setSeries(seriesList,
                               title,
                               "Time",
                               signalName);
    }
}

void StudyBrowserWidget::updateFileInfo()
{
    if (!mFileCountValueLabel ||
        !mLoadedFilesValueLabel)
    {
        return;
    }

    if (mStudies.isEmpty())
    {
        mFileCountValueLabel->setText("0 / 5");
        mLoadedFilesValueLabel->setText("No file loaded");
        return;
    }

    mFileCountValueLabel->setText(
        QString("%1 / 5").arg(mStudies.size()));

    QStringList fileNames;

    for (int i = 0; i < mStudies.size(); ++i)
    {
        fileNames.append(
            QString("%1. %2")
                .arg(i + 1)
                .arg(studyDisplayName(mStudies[i])));
    }

    mLoadedFilesValueLabel->setText(fileNames.join("\n"));
}

PlotType StudyBrowserWidget::currentPlotType() const
{
    if (!mPlotTypeCombo)
        return PlotType::Line;

    return static_cast<PlotType>(
        mPlotTypeCombo->currentData().toInt());
}

const Study* StudyBrowserWidget::referenceStudy() const
{
    if (mStudies.isEmpty())
    {
        return nullptr;
    }

    return mStudies.first();
}

QVector<int> StudyBrowserWidget::selectedStudyIndexes() const
{
    QVector<int> indexes;

    for (int i = 0; i < mFileCheckBoxes.size(); ++i)
    {
        if (mFileCheckBoxes[i] &&
            mFileCheckBoxes[i]->isChecked())
        {
            indexes.append(i);
        }
    }

    return indexes;
}

QString StudyBrowserWidget::studyDisplayName(const Study* study) const
{
    if (!study)
    {
        return QString();
    }

    const QFileInfo fileInfo(study->getFileName());

    if (!fileInfo.fileName().isEmpty())
    {
        return fileInfo.fileName();
    }

    return study->getFileName();
}

void StudyBrowserWidget::rebuildFileSelection()
{
    if (!mFileSelectionLayout)
    {
        return;
    }

    for (QWidget* rowWidget : mFileSelectionRows)
    {
        mFileSelectionLayout->removeWidget(rowWidget);
        rowWidget->deleteLater();
    }

    mFileSelectionRows.clear();
    mFileCheckBoxes.clear();
    mFileColorCombos.clear();
    mFileThicknessCombos.clear();

    for (int i = 0; i < mStudies.size(); ++i)
    {
        const Study* study = mStudies[i];

        QWidget* rowWidget = new QWidget(mFileSelectionFrame);

        QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(6);

        QCheckBox* checkBox =
            new QCheckBox(studyDisplayName(study), rowWidget);

        checkBox->setChecked(i == 0);
        checkBox->setSizePolicy(QSizePolicy::Expanding,
                                QSizePolicy::Preferred);

        QComboBox* colorCombo = new QComboBox(rowWidget);
        configureColorCombo(colorCombo, i);
        colorCombo->setMaximumWidth(100);

        QComboBox* thicknessCombo = new QComboBox(rowWidget);
        thicknessCombo->addItem("1 px", 1);
        thicknessCombo->addItem("2 px", 2);
        thicknessCombo->addItem("3 px", 3);
        thicknessCombo->addItem("4 px", 4);
        thicknessCombo->addItem("5 px", 5);
        thicknessCombo->setCurrentIndex(1);
        thicknessCombo->setMaximumWidth(75);

        connect(checkBox,
                &QCheckBox::toggled,
                this,
                [this](bool)
                {
                    onFileSelectionChanged();
                });

        connect(colorCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                [this](int)
                {
                    onFileStyleChanged();
                });

        connect(thicknessCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                [this](int)
                {
                    onFileStyleChanged();
                });

        rowLayout->addWidget(checkBox, 1);
        rowLayout->addWidget(colorCombo);
        rowLayout->addWidget(thicknessCombo);

        mFileSelectionRows.append(rowWidget);
        mFileCheckBoxes.append(checkBox);
        mFileColorCombos.append(colorCombo);
        mFileThicknessCombos.append(thicknessCombo);

        mFileSelectionLayout->addWidget(rowWidget);
    }
}

void StudyBrowserWidget::configureColorCombo(QComboBox* combo,
                                             int fileIndex) const
{
    if (!combo)
    {
        return;
    }

    combo->clear();

    const QVector<ColorOption> colors =
        comparisonColorOptions();

    for (const ColorOption& option : colors)
    {
        combo->addItem(option.mName, option.mColor);
    }

    if (!colors.isEmpty())
    {
        combo->setCurrentIndex(fileIndex % colors.size());
    }
}

QColor StudyBrowserWidget::colorForFileIndex(int fileIndex) const
{
    if (fileIndex >= 0 &&
        fileIndex < mFileColorCombos.size() &&
        mFileColorCombos[fileIndex])
    {
        const QColor color =
            mFileColorCombos[fileIndex]->currentData().value<QColor>();

        if (color.isValid())
        {
            return color;
        }
    }

    const QVector<ColorOption> colors =
        comparisonColorOptions();

    if (!colors.isEmpty())
    {
        return colors[fileIndex % colors.size()].mColor;
    }

    return QColor();
}

int StudyBrowserWidget::thicknessForFileIndex(int fileIndex) const
{
    if (fileIndex >= 0 &&
        fileIndex < mFileThicknessCombos.size() &&
        mFileThicknessCombos[fileIndex])
    {
        return mFileThicknessCombos[fileIndex]->currentData().toInt();
    }

    return 2;
}

void StudyBrowserWidget::onStudyTypeChanged()
{
    populateComponents();
}

void StudyBrowserWidget::onComponentChanged()
{
    populateSignals();
}

void StudyBrowserWidget::onSignalChanged()
{
    updateSignalSummary();
}

void StudyBrowserWidget::onPlotTypeChanged()
{
    updateSignalSummary();
}

void StudyBrowserWidget::onFileSelectionChanged()
{
    updateSignalSummary();
}

void StudyBrowserWidget::onFileStyleChanged()
{
    updateSignalSummary();
}
