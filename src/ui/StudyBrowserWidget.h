#pragma once

#include <QWidget>
#include "PlotWidget.h"

class QComboBox;
class QLabel;
class QFrame;

class Study;
class PlotWidget;

class StudyBrowserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StudyBrowserWidget(QWidget* parent = nullptr);

    void setStudy(const Study* study);
	void setStudies(const Study* primaryStudy,
                const Study* compareStudy);

private slots:
    void onStudyTypeChanged();
    void onComponentChanged();
    void onSignalChanged();
	void onPlotTypeChanged();

private:
    void populateStudyTypes();
    void populateComponents();
    void populateSignals();
    void updateSignalSummary();
	void updateFileInfo();

	PlotType currentPlotType() const;

private:
    const Study* mPrimaryStudy = nullptr;
	const Study* mCompareStudy = nullptr;

	QFrame* mFileInfoFrame = nullptr;

	QLabel* mFileNameValueLabel = nullptr;
	QLabel* mCompareFileNameValueLabel = nullptr;
	QLabel* mTimePointValueLabel = nullptr;
	QLabel* mStudyTypeValueLabel = nullptr;

    QComboBox* mStudyTypeCombo = nullptr;
    QComboBox* mComponentCombo = nullptr;
    QComboBox* mSignalCombo = nullptr;
	QComboBox* mPlotTypeCombo = nullptr;

    QLabel* mSummaryLabel = nullptr;

    PlotWidget* mPlotWidget = nullptr;
};;

