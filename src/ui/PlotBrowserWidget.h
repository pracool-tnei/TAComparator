#pragma once

#include <QWidget>
#include <QVector>
#include <QPixmap>
#include <QString>
#include <QSize>

#include "FilePlotSettings.h"
#include "PlotWidget.h"

#include "PlotDisplayMapper.h"

class Study;

class QComboBox;
class QLabel;

class QFrame;
class QPushButton;

class PlotBrowserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlotBrowserWidget(QWidget* parent = nullptr);

    void setStudies(const QVector<const Study*>& studies);
    void setFilePlotSettings(const QVector<FilePlotSettings>& settings);
	
	QPixmap exportPlotPixmap(int targetWidth = 1600) const;
	
	void setNameDisplayMode(NameDisplayMode mode);

	void dumpLayoutDebug(const QString& label,
                     int plotIndex) const;

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

	QPixmap exportPlotPixmap(const QSize& exportSize = QSize(1600, 900)) const;
	QString exportPlotTitle() const;

private:
    void populateStudyTypes();
    void populateComponents();
    void populateSignals();

    void updatePlot();

    const Study* referenceStudy() const;

    QString studyDisplayName(const Study* study) const;

    PlotType currentPlotType() const;

	QString currentRawStudyType() const;
	QString currentRawSignalName() const;	
	void clearExportCache();

private slots:
    void onStudyTypeChanged();
    void onComponentChanged();
    void onSignalChanged();
    void onPlotTypeChanged();
	void togglePlotSelectionPanel();

private:
    QVector<const Study*> mStudies;
    QVector<FilePlotSettings> mFilePlotSettings;

    QComboBox* mStudyTypeCombo = nullptr;
    QComboBox* mComponentCombo = nullptr;
    QComboBox* mSignalCombo = nullptr;
    QComboBox* mPlotTypeCombo = nullptr;

    QLabel* mSummaryLabel = nullptr;
    PlotWidget* mPlotWidget = nullptr;

	QFrame* mControlPanel = nullptr;
	QPushButton* mToggleControlsButton = nullptr;

	NameDisplayMode mNameDisplayMode = NameDisplayMode::IpsaFriendly;

	QVector<PlotSeries> mExportSeriesList;
	QString mExportTitle;
	QString mExportXAxisLabel;
	QString mExportYAxisLabel;
	PlotType mExportPlotType = PlotType::Line;
};
