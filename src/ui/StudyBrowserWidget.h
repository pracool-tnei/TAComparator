#pragma once

#include <QWidget>
#include <QVector>
#include <QVBoxLayout>
#include <QColor>
#include "PlotWidget.h"

class QComboBox;
class QLabel;
class QFrame;
class QCheckBox;


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

	void setStudies(const QVector<const Study*>& studies);


private slots:
    void onStudyTypeChanged();
    void onComponentChanged();
    void onSignalChanged();
	void onPlotTypeChanged();
	void onFileSelectionChanged();
	void onFileStyleChanged();

private:
    void populateStudyTypes();
    void populateComponents();
    void populateSignals();
    void updateSignalSummary();
	void updateFileInfo();

	PlotType currentPlotType() const;

	const Study* referenceStudy() const;
	QString studyDisplayName(const Study* study) const;

	void rebuildFileSelection();
	QVector<int> selectedStudyIndexes() const;

	void configureColorCombo(QComboBox* combo,
	                         int fileIndex) const;
	QColor colorForFileIndex(int fileIndex) const;
	int thicknessForFileIndex(int fileIndex) const;

private:
    QVector<const Study*> mStudies;

	QFrame* mFileInfoFrame = nullptr;

	QLabel* mFileCountValueLabel = nullptr;
	QLabel* mLoadedFilesValueLabel = nullptr;

    QComboBox* mStudyTypeCombo = nullptr;
    QComboBox* mComponentCombo = nullptr;
    QComboBox* mSignalCombo = nullptr;
	QComboBox* mPlotTypeCombo = nullptr;

    QLabel* mSummaryLabel = nullptr;

	QFrame* mFileSelectionFrame = nullptr;
	QVBoxLayout* mFileSelectionLayout = nullptr;
	QVector<QCheckBox*> mFileCheckBoxes;

	QVector<QComboBox*> mFileColorCombos;
	QVector<QComboBox*> mFileThicknessCombos;
	QVector<QWidget*> mFileSelectionRows;

    PlotWidget* mPlotWidget = nullptr;
};;

