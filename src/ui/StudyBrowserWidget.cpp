#include "StudyBrowserWidget.h"

#include "../model/Study.h"
#include "PlotWidget.h"

#include <QComboBox>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QFont>


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

	QLabel* fileALabel = new QLabel("Primary file", mFileInfoFrame);
	QLabel* fileBLabel = new QLabel("Compare file", mFileInfoFrame);
	QLabel* timeLabel = new QLabel("Time points", mFileInfoFrame);
	QLabel* studyTypeLabel = new QLabel("Study types", mFileInfoFrame);

	mFileNameValueLabel = new QLabel("No file loaded", mFileInfoFrame);
	mCompareFileNameValueLabel = new QLabel("No compare file", mFileInfoFrame);
	mTimePointValueLabel = new QLabel("-", mFileInfoFrame);
	mStudyTypeValueLabel = new QLabel("-", mFileInfoFrame);

	fileInfoLayout->addWidget(titleLabel, 0, 0, 1, 2);

	fileInfoLayout->addWidget(fileALabel, 1, 0);
	fileInfoLayout->addWidget(mFileNameValueLabel, 1, 1);

	fileInfoLayout->addWidget(fileBLabel, 2, 0);
	fileInfoLayout->addWidget(mCompareFileNameValueLabel, 2, 1);

	fileInfoLayout->addWidget(timeLabel, 3, 0);
	fileInfoLayout->addWidget(mTimePointValueLabel, 3, 1);

	fileInfoLayout->addWidget(studyTypeLabel, 4, 0);
	fileInfoLayout->addWidget(mStudyTypeValueLabel, 4, 1);

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

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* studyTypeLayout = new QHBoxLayout();
	studyTypeLayout->addWidget(new QLabel("Study Type:", this));
	studyTypeLayout->addWidget(mStudyTypeCombo);

    QHBoxLayout* componentLayout = new QHBoxLayout();
    componentLayout->addWidget(new QLabel("Component:", this));
    componentLayout->addWidget(mComponentCombo);

    QHBoxLayout* signalLayout = new QHBoxLayout();
    signalLayout->addWidget(new QLabel("Signal:", this));
    signalLayout->addWidget(mSignalCombo);

	QHBoxLayout* plotTypeLayout = new QHBoxLayout;
	plotTypeLayout->addWidget(new QLabel("Plot Type:", this));
	plotTypeLayout->addWidget(mPlotTypeCombo);

	mainLayout->addWidget(mFileInfoFrame);

	mainLayout->addLayout(studyTypeLayout);
	mainLayout->addLayout(componentLayout);
	mainLayout->addLayout(signalLayout);
	mainLayout->addLayout(plotTypeLayout);

	mainLayout->addWidget(mSummaryLabel);
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
    setStudies(study, nullptr);
}

void StudyBrowserWidget::setStudies(const Study* primaryStudy,
                                    const Study* compareStudy)
{
    mPrimaryStudy = primaryStudy;
    mCompareStudy = compareStudy;

    updateFileInfo();
    populateStudyTypes();
}

void StudyBrowserWidget::populateStudyTypes()
{
    mStudyTypeCombo->clear();
    mComponentCombo->clear();
    mSignalCombo->clear();
    mSummaryLabel->clear();

    if (mPlotWidget)
        mPlotWidget->clear();

    if (!mPrimaryStudy)
        return;

    const QStringList studyTypes =
        mPrimaryStudy->getStudyTypeNames();

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
        mPlotWidget->clear();

    if (!mPrimaryStudy)
        return;

    const QString studyType =
        mStudyTypeCombo->currentText();

    const QString targetGroup =
        mPrimaryStudy->getTargetGroupForStudyType(studyType);

    if (targetGroup.isEmpty())
        return;

    const QList<int> componentIds =
        mPrimaryStudy->getComponentIds(targetGroup);

    for (int componentId : componentIds)
    {
        const QString componentName =
            mPrimaryStudy->getComponentName(targetGroup, componentId);

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
    mSignalCombo->clear();
    mSummaryLabel->clear();

    if (mPlotWidget)
        mPlotWidget->clear();

    if (!mPrimaryStudy)
        return;

    const QString studyType =
        mStudyTypeCombo->currentText();

    if (studyType.isEmpty())
        return;

    const QStringList signalNames =
        mPrimaryStudy->getSignalSchemaNames(studyType, false);

    for (const QString& signalName : signalNames)
    {
        mSignalCombo->addItem(signalName);
    }

    updateSignalSummary();
}

void StudyBrowserWidget::updateSignalSummary()
{
    if (!mPrimaryStudy)
        return;

    const QString studyType =
        mStudyTypeCombo->currentText();

    const QString targetGroup =
        mPrimaryStudy->getTargetGroupForStudyType(studyType);

    const int componentId =
        mComponentCombo->currentData().toInt();

    const QString signalName =
        mSignalCombo->currentText();

    if (studyType.isEmpty() ||
        targetGroup.isEmpty() ||
        signalName.isEmpty())
    {
        mSummaryLabel->clear();

        if (mPlotWidget)
            mPlotWidget->clear();

        return;
    }

    const QVector<double> primaryTimeValues =
        mPrimaryStudy->getTimeValues();

    const QVector<double> primarySignalValues =
        mPrimaryStudy->getSignalValues(targetGroup,
                                       componentId,
                                       signalName);

    QString summary;

    summary += QString("Study Type: %1\n")
                   .arg(studyType);

    summary += QString("Target Group: %1\n")
                   .arg(targetGroup);

    summary += QString("Component ID: %1\n")
                   .arg(componentId);

    summary += QString("Signal: %1\n")
                   .arg(signalName);

    summary += "\nPrimary file:\n";
    summary += QString("  Time count: %1\n")
                   .arg(primaryTimeValues.size());

    summary += QString("  Signal value count: %1\n")
                   .arg(primarySignalValues.size());

    if (!primarySignalValues.isEmpty())
    {
        summary += QString("  First value: %1\n")
                       .arg(primarySignalValues.first());

        summary += QString("  Last value: %1\n")
                       .arg(primarySignalValues.last());
    }

    QVector<PlotSeries> seriesList;

    const QFileInfo primaryFileInfo(mPrimaryStudy->getFileName());

    PlotSeries primarySeries;
    primarySeries.mName =
        primaryFileInfo.fileName().isEmpty()
            ? "Primary"
            : primaryFileInfo.fileName();

    primarySeries.mXValues = primaryTimeValues;
    primarySeries.mYValues = primarySignalValues;

    seriesList.append(primarySeries);

    if (mCompareStudy)
    {
        const QVector<double> compareTimeValues =
            mCompareStudy->getTimeValues();

        const QVector<double> compareSignalValues =
            mCompareStudy->getSignalValues(targetGroup,
                                           componentId,
                                           signalName);

        summary += "\nCompare file:\n";
        summary += QString("  Time count: %1\n")
                       .arg(compareTimeValues.size());

        summary += QString("  Signal value count: %1\n")
                       .arg(compareSignalValues.size());

        if (!compareSignalValues.isEmpty())
        {
            summary += QString("  First value: %1\n")
                           .arg(compareSignalValues.first());

            summary += QString("  Last value: %1\n")
                           .arg(compareSignalValues.last());
        }
        else
        {
            summary += "  Selected signal not available.\n";
        }

        if (!compareTimeValues.isEmpty() &&
            !compareSignalValues.isEmpty() &&
            compareTimeValues.size() == compareSignalValues.size())
        {
            const QFileInfo compareFileInfo(mCompareStudy->getFileName());

            PlotSeries compareSeries;
            compareSeries.mName =
                compareFileInfo.fileName().isEmpty()
                    ? "Compare"
                    : compareFileInfo.fileName();

            compareSeries.mXValues = compareTimeValues;
            compareSeries.mYValues = compareSignalValues;

            seriesList.append(compareSeries);
        }
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
    if (!mFileNameValueLabel ||
        !mCompareFileNameValueLabel ||
        !mTimePointValueLabel ||
        !mStudyTypeValueLabel)
    {
        return;
    }

    if (!mPrimaryStudy)
    {
        mFileNameValueLabel->setText("No file loaded");
        mCompareFileNameValueLabel->setText("No compare file");
        mTimePointValueLabel->setText("-");
        mStudyTypeValueLabel->setText("-");
        return;
    }

    const QFileInfo primaryFileInfo(mPrimaryStudy->getFileName());

    const QString primaryFileName =
        primaryFileInfo.fileName().isEmpty()
            ? mPrimaryStudy->getFileName()
            : primaryFileInfo.fileName();

    mFileNameValueLabel->setText(primaryFileName);

    if (mCompareStudy)
    {
        const QFileInfo compareFileInfo(mCompareStudy->getFileName());

        const QString compareFileName =
            compareFileInfo.fileName().isEmpty()
                ? mCompareStudy->getFileName()
                : compareFileInfo.fileName();

        mCompareFileNameValueLabel->setText(compareFileName);
    }
    else
    {
        mCompareFileNameValueLabel->setText("No compare file");
    }

    mTimePointValueLabel->setText(
        QString::number(mPrimaryStudy->getTimeValues().size()));

    mStudyTypeValueLabel->setText(
        QString::number(mPrimaryStudy->getStudyTypeNames().size()));
}

PlotType StudyBrowserWidget::currentPlotType() const
{
    if (!mPlotTypeCombo)
        return PlotType::Line;

    return static_cast<PlotType>(
        mPlotTypeCombo->currentData().toInt());
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
