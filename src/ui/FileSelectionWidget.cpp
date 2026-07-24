#include "FileSelectionWidget.h"

#include "../model/Study.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QColorDialog>
#include <QSignalBlocker>

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

	constexpr int CustomColorRole = Qt::UserRole + 1;
}

FileSelectionWidget::FileSelectionWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(10);

    //
    // Current Study card
    //
    QFrame* fileInfoFrame = new QFrame(this);
    fileInfoFrame->setFrameShape(QFrame::StyledPanel);
    fileInfoFrame->setFrameShadow(QFrame::Raised);

    QGridLayout* fileInfoLayout = new QGridLayout(fileInfoFrame);
    fileInfoLayout->setContentsMargins(12, 10, 12, 10);
    fileInfoLayout->setHorizontalSpacing(16);
    fileInfoLayout->setVerticalSpacing(6);

    QLabel* titleLabel = new QLabel("Current Study", fileInfoFrame);

    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    titleLabel->setFont(titleFont);

    QLabel* fileCountLabel = new QLabel("Files", fileInfoFrame);
    QLabel* loadedFilesLabel = new QLabel("Loaded files", fileInfoFrame);

    mFileCountValueLabel = new QLabel("0 / 5", fileInfoFrame);

    mLoadedFilesValueLabel = new QLabel("No file loaded", fileInfoFrame);
    mLoadedFilesValueLabel->setWordWrap(true);

    fileInfoLayout->addWidget(titleLabel, 0, 0, 1, 2);
    fileInfoLayout->addWidget(fileCountLabel, 1, 0);
    fileInfoLayout->addWidget(mFileCountValueLabel, 1, 1);
    fileInfoLayout->addWidget(loadedFilesLabel, 2, 0);
    fileInfoLayout->addWidget(mLoadedFilesValueLabel, 2, 1);

    fileInfoLayout->setColumnStretch(1, 1);

    mainLayout->addWidget(fileInfoFrame);

    //
    // Files to Plot card
    //
    mFileSelectionFrame = new QFrame(this);
    mFileSelectionFrame->setFrameShape(QFrame::StyledPanel);

    mFileSelectionLayout = new QVBoxLayout(mFileSelectionFrame);
    mFileSelectionLayout->setContentsMargins(10, 10, 10, 10);
    mFileSelectionLayout->setSpacing(6);

    QLabel* fileSelectionTitle =
        new QLabel("Files to Plot", mFileSelectionFrame);

    QFont fileSelectionTitleFont = fileSelectionTitle->font();
    fileSelectionTitleFont.setBold(true);
    fileSelectionTitle->setFont(fileSelectionTitleFont);

    mFileSelectionLayout->addWidget(fileSelectionTitle);

    mainLayout->addWidget(mFileSelectionFrame);
    mainLayout->addStretch();
}

void FileSelectionWidget::setStudies(const QVector<const Study*>& studies)
{
    mStudies = studies;

    updateFileInfo();
    rebuildFileSelection();

    emit settingsChanged();
}

void FileSelectionWidget::updateFileInfo()
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

void FileSelectionWidget::rebuildFileSelection()
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
                    emit settingsChanged();
                });

		connect(colorCombo,
				QOverload<int>::of(&QComboBox::currentIndexChanged),
				this,
				[this, colorCombo](int index)
				{
					if (!colorCombo)
					{
						return;
					}
		
					const bool isCustomColor =
						colorCombo->itemData(index,
											 CustomColorRole).toBool();
		
					if (!isCustomColor)
					{
						const QColor selectedColor =
							colorCombo->currentData().value<QColor>();
		
						if (selectedColor.isValid())
						{
							colorCombo->setProperty("selectedColor",
													selectedColor);
						}
		
						colorCombo->setProperty("previousColorIndex",
												index);
		
						emit settingsChanged();
						return;
					}
		
					QColor initialColor =
						colorCombo->property("selectedColor").value<QColor>();
		
					if (!initialColor.isValid())
					{
						initialColor = QColor(0, 90, 180);
					}
		
					const QColor chosenColor =
						QColorDialog::getColor(initialColor,
											   this,
											   "Select Plot Colour");
		
					if (!chosenColor.isValid())
					{
						const int previousIndex =
							colorCombo->property("previousColorIndex").toInt();
		
						QSignalBlocker blocker(colorCombo);
		
						if (previousIndex >= 0 &&
							previousIndex < colorCombo->count())
						{
							colorCombo->setCurrentIndex(previousIndex);
						}
		
						return;
					}
		
					{
						QSignalBlocker blocker(colorCombo);
		
						colorCombo->setItemText(
							index,
							chosenColor.name().toUpper());
		
						colorCombo->setItemData(index,
												chosenColor);
		
						colorCombo->setItemData(index,
												true,
												CustomColorRole);
		
						colorCombo->setCurrentIndex(index);
					}
		
					colorCombo->setProperty("selectedColor",
											chosenColor);
		
					colorCombo->setProperty("previousColorIndex",
											index);
		
					emit settingsChanged();
				});


        connect(thicknessCombo,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                [this](int)
                {
                    emit settingsChanged();
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

QVector<int> FileSelectionWidget::selectedStudyIndexes() const
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

QVector<FilePlotSettings> FileSelectionWidget::filePlotSettings() const
{
    QVector<FilePlotSettings> settings;

    for (int i = 0; i < mStudies.size(); ++i)
    {
        FilePlotSettings item;
        item.mEnabled = false;
        item.mColor = colorForFileIndex(i);
        item.mLineThickness = thicknessForFileIndex(i);

        if (i < mFileCheckBoxes.size() &&
            mFileCheckBoxes[i])
        {
            item.mEnabled =
                mFileCheckBoxes[i]->isChecked();
        }

        settings.append(item);
    }

    return settings;
}

void FileSelectionWidget::configureColorCombo(QComboBox* combo,
                                              int fileIndex) const
{
    if (!combo)
    {
        return;
    }

    combo->clear();

    /*
     * Custom colour appears first in the dropdown.
     * But it is NOT selected by default.
     */
    combo->addItem("Custom...",
                   QColor());

    combo->setItemData(combo->count() - 1,
                       true,
                       CustomColorRole);

    const QVector<ColorOption> colors =
        comparisonColorOptions();

    for (const ColorOption& option : colors)
    {
        combo->addItem(option.mName,
                       option.mColor);

        combo->setItemData(combo->count() - 1,
                           false,
                           CustomColorRole);
    }

    if (!colors.isEmpty())
    {
        const int defaultColorIndex =
            fileIndex % colors.size();

        /*
         * +1 because index 0 is now Custom...
         */
        const int comboIndex =
            defaultColorIndex + 1;

        combo->setCurrentIndex(comboIndex);

        combo->setProperty("previousColorIndex",
                           comboIndex);

        combo->setProperty("selectedColor",
                           colors[defaultColorIndex].mColor);
    }
}

QColor FileSelectionWidget::colorForFileIndex(int fileIndex) const
{
    if (fileIndex >= 0 &&
        fileIndex < mFileColorCombos.size() &&
        mFileColorCombos[fileIndex])
    {
		const QColor color =
			mFileColorCombos[fileIndex]
				->currentData()
				.value<QColor>();
		
		if (color.isValid())
		{
			return color;
		}
		
		const QColor storedColor =
			mFileColorCombos[fileIndex]
				->property("selectedColor")
				.value<QColor>();
		
		if (storedColor.isValid())
		{
			return storedColor;
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

int FileSelectionWidget::thicknessForFileIndex(int fileIndex) const
{
    if (fileIndex >= 0 &&
        fileIndex < mFileThicknessCombos.size() &&
        mFileThicknessCombos[fileIndex])
    {
        return mFileThicknessCombos[fileIndex]
            ->currentData()
            .toInt();
    }

    return 2;
}

QString FileSelectionWidget::studyDisplayName(const Study* study) const
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
