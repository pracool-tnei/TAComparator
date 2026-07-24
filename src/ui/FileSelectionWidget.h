#pragma once

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QStringList>

#include "FilePlotSettings.h"

class Study;

class QLabel;
class QFrame;
class QVBoxLayout;
class QCheckBox;
class QComboBox;

class FileSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileSelectionWidget(QWidget* parent = nullptr);

    void setStudies(const QVector<const Study*>& studies);

    QVector<int> selectedStudyIndexes() const;
    QVector<FilePlotSettings> filePlotSettings() const;

    QColor colorForFileIndex(int fileIndex) const;
    int thicknessForFileIndex(int fileIndex) const;
	void setPendingSelectedFileNames(
    const QStringList& selectedFileNames);
	
signals:
    void settingsChanged();
	void removeStudyRequested(int studyIndex);

private:
    void updateFileInfo();
    void rebuildFileSelection();

    void configureColorCombo(QComboBox* combo,
                             int fileIndex) const;

    QString studyDisplayName(const Study* study) const;

private:
    QVector<const Study*> mStudies;

    QLabel* mFileCountValueLabel = nullptr;
    QLabel* mLoadedFilesValueLabel = nullptr;

    QFrame* mFileSelectionFrame = nullptr;
    QVBoxLayout* mFileSelectionLayout = nullptr;

    QVector<QWidget*> mFileSelectionRows;
    QVector<QCheckBox*> mFileCheckBoxes;
    QVector<QComboBox*> mFileColorCombos;
    QVector<QComboBox*> mFileThicknessCombos;

	QStringList mPendingSelectedFileNames;
	bool mUsePendingSelectedFileNames = false;
};
