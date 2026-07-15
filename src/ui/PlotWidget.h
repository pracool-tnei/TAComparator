#pragma once

#include <QWidget>
#include <QVector>
#include <QString>

enum class PlotType
{
    Line,
    Bar
};

struct PlotSeries
{
    QString mName;
    QVector<double> mXValues;
    QVector<double> mYValues;
};

class PlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlotWidget(QWidget* parent = nullptr);

    void setData(const QVector<double>& xValues,
                 const QVector<double>& yValues,
                 const QString& title,
                 const QString& xAxisLabel,
                 const QString& yAxisLabel);

    void setSeries(const QVector<PlotSeries>& series,
                   const QString& title,
                   const QString& xAxisLabel,
                   const QString& yAxisLabel);

    void setPlotType(PlotType plotType);

    PlotType getPlotType() const;

    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<PlotSeries> mSeries;

    QString mTitle;
    QString mXAxisLabel;
    QString mYAxisLabel;

    PlotType mPlotType = PlotType::Line;
};
