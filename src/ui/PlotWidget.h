#pragma once

#include <QWidget>
#include <QVector>
#include <QString>
#include <QColor>
#include <QPoint>

class QEvent;
class QWheelEvent;
class QMouseEvent;

enum class PlotType
{
    Line,
    DashedLine,
    DottedLine,
    DashDotLine,
    Bar
};

struct PlotSeries
{
    QString mName;
    QVector<double> mXValues;
    QVector<double> mYValues;

    QColor mColor;
    int mLineThickness = 2;
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
	void setShowMajorGrid(bool show);
	void setShowMinorGrid(bool show);
	
	bool showMajorGrid() const;
	bool showMinorGrid() const;



protected:
	void paintEvent(QPaintEvent* event) override;

	// Hover / pan
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void leaveEvent(QEvent* event) override;

	// Zoom
	void wheelEvent(QWheelEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;

	
//zoom
private:
	QRect plotAreaRect() const;
	bool dataXRange(double& minX,
	                double& maxX) const;
	void resetZoom();

private:
    QVector<PlotSeries> mSeries;

    QString mTitle;
    QString mXAxisLabel;
    QString mYAxisLabel;

	//hover for details
    QPoint mMousePosition;
    bool mHasMousePosition = false;

	//zoom
	bool mHasCustomXRange = false;
	double mCustomMinX = 0.0;
	double mCustomMaxX = 0.0;

	// Pan
    bool mIsPanning = false;
    QPoint mPanStartMousePosition;
    double mPanStartMinX = 0.0;
    double mPanStartMaxX = 0.0;

	bool mShowMajorGrid = true;
	bool mShowMinorGrid = true;

    PlotType mPlotType = PlotType::Line;
};
