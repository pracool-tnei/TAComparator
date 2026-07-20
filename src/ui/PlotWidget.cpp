#include "PlotWidget.h"

#include <QPainter>
#include <QPen>
#include <QFontMetrics>
#include <QPolygonF>
#include <QColor>

//hover for details
#include <QMouseEvent>
#include <QFont>

//zoom
#include <QWheelEvent>
#include <QMouseEvent>

#include <algorithm>
#include <cmath>

namespace
{
    QString formatAxisValue(double value)
    {
        return QString::number(value, 'g', 6);
    }

    QColor seriesColor(int index)
	{
	    static const QVector<QColor> colors =
	    {
	        QColor(0, 90, 180),
	        QColor(190, 60, 50),
	        QColor(40, 140, 80),
	        QColor(120, 80, 170),
	        QColor(200, 130, 30)
	    };

	    return colors[index % colors.size()];
	}

	double niceNumber(double value,
	                  bool round)
	{
	    if (value <= 0.0)
	        return 1.0;

	    const double exponent =
	        std::floor(std::log10(value));

	    const double fraction =
	        value / std::pow(10.0, exponent);

	    double niceFraction = 1.0;

	    if (round)
	    {
	        if (fraction < 1.5)
	            niceFraction = 1.0;
	        else if (fraction < 3.0)
	            niceFraction = 2.0;
	        else if (fraction < 7.0)
	            niceFraction = 5.0;
	        else
	            niceFraction = 10.0;
	    }
	    else
	    {
	        if (fraction <= 1.0)
	            niceFraction = 1.0;
	        else if (fraction <= 2.0)
	            niceFraction = 2.0;
	        else if (fraction <= 5.0)
	            niceFraction = 5.0;
	        else
	            niceFraction = 10.0;
	    }

	    return niceFraction * std::pow(10.0, exponent);
	}

	void applyAutomaticYAxisRange(double& minY,
                              double& maxY,
                              PlotType plotType,
                              int tickCount)
	{
	    if (plotType == PlotType::Bar)
	    {
	        const double rawMinY = minY;
	        const double rawMaxY = maxY;

	        //
	        // All-positive values, including all-zero values.
	        // Bar charts should start from zero.
	        //
	        if (rawMinY >= 0.0 && rawMaxY >= 0.0)
	        {
	            minY = 0.0;

	            if (rawMaxY == 0.0)
	            {
	                //
	                // Do not use 1.0 for all-zero small engineering values.
	                // This keeps zero-valued resistance/reactance plots from
	                // looking artificially huge.
	                //
	                maxY = 0.01;
	            }
	            else
	            {
	                maxY = niceNumber(rawMaxY * 1.15, false);
	            }

	            return;
	        }

	        //
	        // All-negative values.
	        // Bar charts should end at zero.
	        //
	        if (rawMinY <= 0.0 && rawMaxY <= 0.0)
	        {
	            maxY = 0.0;

	            if (rawMinY == 0.0)
	            {
	                minY = -0.01;
	            }
	            else
	            {
	                minY = -niceNumber(std::abs(rawMinY) * 1.15, false);
	            }

	            return;
	        }

	        //
	        // Mixed positive and negative values.
	        // Keep zero inside the chart.
	        //
	        minY = -niceNumber(std::abs(rawMinY) * 1.15, false);
	        maxY = niceNumber(std::abs(rawMaxY) * 1.15, false);

	        return;
	    }

	    //
	    // Line chart:
	    // Do not force zero, because transient plots need visible variation.
	    //
	    if (minY == maxY)
	    {
	        const double magnitude =
	            std::abs(minY);

	        const double padding =
	            magnitude > 0.0
	                ? magnitude * 0.10
	                : 1.0;

	        minY -= padding;
	        maxY += padding;
	    }
	    else
	    {
	        const double range =
	            maxY - minY;

	        const double padding =
	            range * 0.08;

	        minY -= padding;
	        maxY += padding;
	    }

	    const double range =
	        maxY - minY;

	    const double tickStep =
	        niceNumber(range / tickCount, true);

	    minY =
	        std::floor(minY / tickStep) * tickStep;

	    maxY =
	        std::ceil(maxY / tickStep) * tickStep;

	    if (minY == maxY)
	    {
	        minY -= 1.0;
	        maxY += 1.0;
	    }
	}

	QVector<double> generateNiceTicks(double minValue,
									  double maxValue,
									  int targetTickCount)
	{
		QVector<double> ticks;
	
		if (targetTickCount <= 0)
		{
			return ticks;
		}
	
		if (maxValue <= minValue)
		{
			ticks.append(minValue);
			return ticks;
		}
	
		const double range =
			maxValue - minValue;
	
		const double rawStep =
			range / static_cast<double>(targetTickCount);
	
		const double step =
			niceNumber(rawStep, true);
	
		if (step <= 0.0)
		{
			return ticks;
		}
	
		const double firstTick =
			std::ceil(minValue / step) * step;
	
		for (double value = firstTick;
			 value <= maxValue + step * 0.5;
			 value += step)
		{
			if (value >= minValue - step * 0.5 &&
				value <= maxValue + step * 0.5)
			{
				ticks.append(value);
			}
		}
	
		return ticks;
	}
	QVector<double> generateMinorTicks(double minValue,
									   double maxValue,
									   const QVector<double>& majorTicks,
									   int subdivisions)
	{
		QVector<double> minorTicks;
	
		if (subdivisions <= 1 ||
			majorTicks.size() < 2 ||
			maxValue <= minValue)
		{
			return minorTicks;
		}
	
		const double majorStep =
			majorTicks[1] - majorTicks[0];
	
		if (majorStep <= 0.0)
		{
			return minorTicks;
		}
	
		const double minorStep =
			majorStep / static_cast<double>(subdivisions);
	
		if (minorStep <= 0.0)
		{
			return minorTicks;
		}
	
		const double firstTick =
			std::ceil(minValue / minorStep) * minorStep;
	
		for (double value = firstTick;
			 value <= maxValue + minorStep * 0.5;
			 value += minorStep)
		{
			bool isMajorTick = false;
	
			for (double majorTick : majorTicks)
			{
				if (std::abs(value - majorTick) < minorStep * 0.25)
				{
					isMajorTick = true;
					break;
				}
			}
	
			if (!isMajorTick &&
				value >= minValue - minorStep * 0.5 &&
				value <= maxValue + minorStep * 0.5)
			{
				minorTicks.append(value);
			}
		}
	
		return minorTicks;
	}

}

PlotWidget::PlotWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(0, 0);

    setSizePolicy(QSizePolicy::Ignored,
                  QSizePolicy::Ignored);

    setMouseTracking(true);
}


void PlotWidget::setData(const QVector<double>& xValues,
                         const QVector<double>& yValues,
                         const QString& title,
                         const QString& xAxisLabel,
                         const QString& yAxisLabel)
{
    PlotSeries series;
    series.mName = "Series";
    series.mXValues = xValues;
    series.mYValues = yValues;

    setSeries({ series },
              title,
              xAxisLabel,
              yAxisLabel);
}

void PlotWidget::setSeries(const QVector<PlotSeries>& series,
                           const QString& title,
                           const QString& xAxisLabel,
                           const QString& yAxisLabel)
{
	mSeries = series;
	mTitle = title;
	mXAxisLabel = xAxisLabel;
	mYAxisLabel = yAxisLabel;
	
	mHasCustomXRange = false;
	mCustomMinX = 0.0;
	mCustomMaxX = 0.0;

	mIsPanning = false;
	unsetCursor();

	update();
}

void PlotWidget::setPlotType(PlotType plotType)
{
    mPlotType = plotType;
    update();
}

PlotType PlotWidget::getPlotType() const
{
    return mPlotType;
}

void PlotWidget::clear()
{
    mSeries.clear();
    mTitle.clear();
    mXAxisLabel.clear();
    mYAxisLabel.clear();

    mHasCustomXRange = false;
	mCustomMinX = 0.0;
	mCustomMaxX = 0.0;

	mIsPanning = false;
	unsetCursor();

	update();
}

void PlotWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), palette().window());

	QRect plotRect = plotAreaRect();

    if (plotRect.width() <= 0 || plotRect.height() <= 0)
        return;

    painter.setPen(QPen(palette().text().color(), 1));

    if (!mTitle.isEmpty())
    {
        painter.drawText(QRect(0, 5, width(), 25),
                         Qt::AlignCenter,
                         mTitle);
    }

	if (mHasCustomXRange)
	{
	    painter.drawText(QRect(plotRect.left(),
	                           22,
	                           plotRect.width(),
	                           18),
	                     Qt::AlignRight,
	                     "Zoomed - drag to pan, double-click plot to reset");
	}

    painter.drawRect(plotRect);

    QVector<PlotSeries> validSeries;

    for (const PlotSeries& series : mSeries)
    {
        if (!series.mXValues.isEmpty() &&
            !series.mYValues.isEmpty() &&
            series.mXValues.size() == series.mYValues.size())
        {
            validSeries.append(series);
        }
    }

    if (validSeries.isEmpty())
    {
        painter.drawText(plotRect,
                         Qt::AlignCenter,
                         "No data to plot");
        return;
    }

    bool firstValue = true;

    double minX = 0.0;
    double maxX = 0.0;
    double minY = 0.0;
    double maxY = 0.0;

    for (const PlotSeries& series : validSeries)
    {
        auto minMaxX =
            std::minmax_element(series.mXValues.begin(),
                                series.mXValues.end());

        auto minMaxY =
            std::minmax_element(series.mYValues.begin(),
                                series.mYValues.end());

        if (firstValue)
        {
            minX = *minMaxX.first;
            maxX = *minMaxX.second;
            minY = *minMaxY.first;
            maxY = *minMaxY.second;
            firstValue = false;
        }
        else
        {
            minX = std::min(minX, *minMaxX.first);
            maxX = std::max(maxX, *minMaxX.second);
            minY = std::min(minY, *minMaxY.first);
            maxY = std::max(maxY, *minMaxY.second);
        }
    }

    if (minX == maxX)
    {
        minX -= 1.0;
        maxX += 1.0;
    }

	if (mHasCustomXRange)
	{
	    minX = mCustomMinX;
	    maxX = mCustomMaxX;
	}

	double axisMinX = minX;
	double axisMaxX = maxX;

	if (mPlotType == PlotType::Bar)
	{
	    int maxPointCountForPadding = 0;

	    for (const PlotSeries& series : validSeries)
	    {
	        maxPointCountForPadding =
	            std::max(maxPointCountForPadding,
	                     static_cast<int>(series.mXValues.size()));
	    }

	    const double xStep =
	        maxPointCountForPadding > 1
	            ? (maxX - minX) / (maxPointCountForPadding - 1)
	            : 1.0;

	    minX -= xStep * 0.50;
	    maxX += xStep * 0.50;
	}

    const int tickCount = 5;
	const int tickSize = 5;

	applyAutomaticYAxisRange(minY,
	                         maxY,
	                         mPlotType,
	                         tickCount);

    const auto mapPoint = [&](double x, double y) -> QPointF
    {
        const double px =
            plotRect.left()
            + ((x - minX) / (maxX - minX)) * plotRect.width();

        const double py =
            plotRect.bottom()
            - ((y - minY) / (maxY - minY)) * plotRect.height();

        return QPointF(px, py);
    };

	QFontMetrics metrics(painter.font());
	
	QPen axisPen(palette().text().color(), 1);
	axisPen.setCosmetic(true);
	
	QPen gridPen(palette().mid().color(), 1, Qt::DotLine);
	gridPen.setCosmetic(true);
	
	QColor minorGridColor =
		palette().mid().color();
	
	minorGridColor.setAlpha(80);
	
	QPen minorGridPen(minorGridColor, 1, Qt::DotLine);
	minorGridPen.setCosmetic(true);
	
	painter.setPen(axisPen);

	const QVector<double> xTicks =
		generateNiceTicks(axisMinX,
						  axisMaxX,
						  8);
	
	const QVector<double> minorXTicks =
		generateMinorTicks(axisMinX,
						   axisMaxX,
						   xTicks,
						   5);
	
	const QVector<double> yTicks =
		generateNiceTicks(minY,
						  maxY,
						  6);
	
	const QVector<double> minorYTicks =
		generateMinorTicks(minY,
						   maxY,
						   yTicks,
						   5);

	//
	// Minor vertical grid lines.
	//
	painter.setPen(minorGridPen);
	
	for (double value : minorXTicks)
	{
		const double ratio =
			(value - minX) / (maxX - minX);
	
		const int x =
			plotRect.left()
			+ static_cast<int>(ratio * plotRect.width());
	
		if (x <= plotRect.left() ||
			x >= plotRect.right())
		{
			continue;
		}
	
		painter.drawLine(x,
						 plotRect.top(),
						 x,
						 plotRect.bottom());
	}
	
	//
	// Minor horizontal grid lines.
	//
	for (double value : minorYTicks)
	{
		const double ratio =
			(value - minY) / (maxY - minY);
	
		const int y =
			plotRect.bottom()
			- static_cast<int>(ratio * plotRect.height());
	
		if (y <= plotRect.top() ||
			y >= plotRect.bottom())
		{
			continue;
		}
	
		painter.drawLine(plotRect.left(),
						 y,
						 plotRect.right(),
						 y);
	}

	
	for (double value : xTicks)
	{
		const double ratio =
			(value - minX) / (maxX - minX);
	
		const int x =
			plotRect.left()
			+ static_cast<int>(ratio * plotRect.width());
	
		if (x < plotRect.left() ||
			x > plotRect.right())
		{
			continue;
		}
	
		//
		// Vertical grid line inside plot area.
		//
		if (x > plotRect.left() &&
			x < plotRect.right())
		{
			painter.setPen(gridPen);
	
			painter.drawLine(x,
							 plotRect.top(),
							 x,
							 plotRect.bottom());
		}
	
		painter.setPen(axisPen);
	
		painter.drawLine(x,
						 plotRect.bottom(),
						 x,
						 plotRect.bottom() + tickSize);
	
		const QString text =
			formatAxisValue(value);
	
		const int textWidth =
			metrics.horizontalAdvance(text);
	
		painter.drawText(x - textWidth / 2,
						 plotRect.bottom() + tickSize + metrics.height(),
						 text);
	}

	
	for (double value : yTicks)
	{
		const double ratio =
			(value - minY) / (maxY - minY);
	
		const int y =
			plotRect.bottom()
			- static_cast<int>(ratio * plotRect.height());
	
		if (y < plotRect.top() ||
			y > plotRect.bottom())
		{
			continue;
		}
	
		//
		// Horizontal grid line inside plot area.
		//
		if (y > plotRect.top() &&
			y < plotRect.bottom())
		{
			painter.setPen(gridPen);
	
			painter.drawLine(plotRect.left(),
							 y,
							 plotRect.right(),
							 y);
		}
	
		painter.setPen(axisPen);
	
		painter.drawLine(plotRect.left() - tickSize,
						 y,
						 plotRect.left(),
						 y);
	
		const QString text =
			formatAxisValue(value);
	
		const int textWidth =
			metrics.horizontalAdvance(text);
	
		painter.drawText(plotRect.left() - tickSize - textWidth - 6,
						 y + metrics.height() / 3,
						 text);
	}

	painter.setPen(axisPen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(plotRect);

    if (!mXAxisLabel.isEmpty())
	{
	    painter.drawText(QRect(plotRect.left(),
	                           height() - 45,
	                           plotRect.width(),
	                           25),
	                     Qt::AlignCenter,
	                     mXAxisLabel);
	}

    if (!mYAxisLabel.isEmpty())
    {
        painter.save();

        painter.translate(20,
                          plotRect.top() + plotRect.height() / 2);

        painter.rotate(-90);

        painter.drawText(QRect(-plotRect.height() / 2,
                               -10,
                               plotRect.height(),
                               20),
                         Qt::AlignCenter,
                         mYAxisLabel);

        painter.restore();
    }

    painter.save();
	painter.setClipRect(plotRect.adjusted(1, 1, -1, -1));

	if (mPlotType == PlotType::Line)
	{
	    for (int seriesIndex = 0;
	         seriesIndex < validSeries.size();
	         ++seriesIndex)
	    {
	        const PlotSeries& series = validSeries[seriesIndex];

	        QPolygonF polyline;

	        for (int i = 0; i < series.mXValues.size(); ++i)
	        {
	            polyline.append(mapPoint(series.mXValues[i],
	                                     series.mYValues[i]));
	        }

	        const QColor color =
			    series.mColor.isValid()
			        ? series.mColor
			        : seriesColor(seriesIndex);

			const int thickness =
			    std::max(1, series.mLineThickness);

			painter.setPen(QPen(color, thickness));
	        painter.drawPolyline(polyline);
	    }
	}
	else if (mPlotType == PlotType::Bar)
	{
	    int maxPointCount = 0;

	    for (const PlotSeries& series : validSeries)
	    {
	        maxPointCount = std::max(maxPointCount,
	                                 static_cast<int>(series.mXValues.size()));
	    }

	    if (maxPointCount > 0)
	    {
	        const double groupWidth =
	            std::max(2.0,
	                     std::min(28.0,
	                              plotRect.width()
	                              / static_cast<double>(maxPointCount)
	                              * 0.70));

	        const double barWidth =
	            std::max(1.0,
	                     groupWidth / validSeries.size());

	        double baselineValue = 0.0;

	        if (baselineValue < minY)
	            baselineValue = minY;

	        if (baselineValue > maxY)
	            baselineValue = maxY;

	        for (int seriesIndex = 0;
	             seriesIndex < validSeries.size();
	             ++seriesIndex)
	        {
	            const PlotSeries& series = validSeries[seriesIndex];

	            const QColor color =
				    series.mColor.isValid()
				        ? series.mColor
				        : seriesColor(seriesIndex);

				painter.setPen(QPen(color, 1));
				painter.setBrush(color);

	            for (int i = 0; i < series.mXValues.size(); ++i)
	            {
	                const QPointF valuePoint =
	                    mapPoint(series.mXValues[i],
	                             series.mYValues[i]);

	                const QPointF baselinePoint =
	                    mapPoint(series.mXValues[i],
	                             baselineValue);

	                const double groupLeft =
	                    valuePoint.x() - groupWidth / 2.0;

	                const double barLeft =
	                    groupLeft + seriesIndex * barWidth;

	                const double top =
	                    std::min(valuePoint.y(),
	                             baselinePoint.y());

	                const double height =
	                    std::abs(valuePoint.y() -
	                             baselinePoint.y());

	                QRectF barRect(barLeft,
	                               top,
	                               barWidth * 0.85,
	                               height);

	                painter.drawRect(barRect);
	            }

	            painter.setBrush(Qt::NoBrush);
	        }
	    }
	}

	painter.restore();

	//
	// Hover details.
	// Comparison-friendly behavior:
	//	 1. Find nearest time sample on X-axis.
	//	 2. Show values for all currently plotted series at that time.
	//
	if (mHasMousePosition &&
		plotRect.contains(mMousePosition))
	{
		bool foundTime = false;
	
		double nearestTime = 0.0;
		double nearestXDistanceSquared = 0.0;
		double guideX = 0.0;
	
		//
		// First pass:
		// Find the nearest X/time sample to the mouse position.
		// We intentionally use X-distance only, not Y-distance.
		// This makes hover useful anywhere vertically inside the plot.
		//
		for (const PlotSeries& series : validSeries)
		{
			for (int pointIndex = 0;
				 pointIndex < series.mXValues.size();
				 ++pointIndex)
			{
				const QPointF point =
					mapPoint(series.mXValues[pointIndex],
							 series.mYValues[pointIndex]);
	
				const double dx =
					point.x() - mMousePosition.x();
	
				const double distanceSquared =
					dx * dx;
	
				if (!foundTime ||
					distanceSquared < nearestXDistanceSquared)
				{
					foundTime = true;
					nearestXDistanceSquared = distanceSquared;
					nearestTime = series.mXValues[pointIndex];
					guideX = point.x();
				}
			}
		}
	
		if (foundTime)
		{
			struct HoverValue
			{
				QString mName;
				double mTime = 0.0;
				double mValue = 0.0;
				QPointF mPoint;
				QColor mColor;
				bool mValid = false;
			};
	
			QVector<HoverValue> hoverValues;
	
			//
			// Second pass:
			// For every plotted series, find the value closest to nearestTime.
			//
			for (int seriesIndex = 0;
				 seriesIndex < validSeries.size();
				 ++seriesIndex)
			{
				const PlotSeries& series =
					validSeries[seriesIndex];
	
				int nearestIndex = -1;
				double nearestTimeDelta = 0.0;
				bool foundPointForSeries = false;
	
				for (int pointIndex = 0;
					 pointIndex < series.mXValues.size();
					 ++pointIndex)
				{
					const double timeDelta =
						std::abs(series.mXValues[pointIndex] - nearestTime);
	
					if (!foundPointForSeries ||
						timeDelta < nearestTimeDelta)
					{
						foundPointForSeries = true;
						nearestTimeDelta = timeDelta;
						nearestIndex = pointIndex;
					}
				}
	
				if (nearestIndex < 0)
				{
					continue;
				}
	
				HoverValue hoverValue;
				hoverValue.mName = series.mName;
				hoverValue.mTime = series.mXValues[nearestIndex];
				hoverValue.mValue = series.mYValues[nearestIndex];
				hoverValue.mPoint = mapPoint(hoverValue.mTime,
											 hoverValue.mValue);
				hoverValue.mColor =
					series.mColor.isValid()
						? series.mColor
						: seriesColor(seriesIndex);
				hoverValue.mValid = true;
	
				hoverValues.append(hoverValue);
			}
	
			if (!hoverValues.isEmpty())
			{
				//
				// Vertical guide line.
				//
				painter.setPen(QPen(palette().mid().color(),
									1,
									Qt::DashLine));
	
				painter.drawLine(QPointF(guideX, plotRect.top()),
								 QPointF(guideX, plotRect.bottom()));
	
				//
				// Mark all series points at this time.
				//
				for (const HoverValue& hoverValue : hoverValues)
				{
					if (!hoverValue.mValid)
					{
						continue;
					}
	
					painter.setPen(QPen(hoverValue.mColor, 2));
					painter.setBrush(palette().window());
	
					painter.drawEllipse(hoverValue.mPoint,
										5,
										5);
				}
	
				//
				// Tooltip content.
				// Keep numeric values fully visible.
				// Only file names are allowed to shorten if space is tight.
				//
				struct HoverDisplayRow
				{
				    QString mName;
				    QString mValue;
				    QColor mColor;
				};

				QVector<HoverDisplayRow> displayRows;

				for (const HoverValue& hoverValue : hoverValues)
				{
				    HoverDisplayRow row;
				    row.mName = hoverValue.mName;
				    row.mValue = QString::number(hoverValue.mValue, 'g', 10);
				    row.mColor = hoverValue.mColor;

				    displayRows.append(row);
				}

				const QString timeLine =
				    QString("Time: %1 s")
				        .arg(QString::number(nearestTime, 'g', 10));

				const QFont originalFont = painter.font();

				QFont tooltipFont = painter.font();
				tooltipFont.setPointSize(tooltipFont.pointSize() - 1);
				painter.setFont(tooltipFont);

				QFontMetrics tooltipMetrics(painter.font());

				const int padding = 8;
				const int rowGap = 6;
				const int colorMarkerWidth = 12;
				const int markerToNameGap = 6;
				const int nameToValueGap = 12;

				int maxNameWidth = 0;
				int maxValueWidth = 0;

				for (const HoverDisplayRow& row : displayRows)
				{
				    maxNameWidth =
				        std::max(maxNameWidth,
				                 tooltipMetrics.horizontalAdvance(row.mName));

				    maxValueWidth =
				        std::max(maxValueWidth,
				                 tooltipMetrics.horizontalAdvance(row.mValue));
				}

				const int timeWidth =
				    tooltipMetrics.horizontalAdvance(timeLine);

				int naturalContentWidth =
				    colorMarkerWidth
				    + markerToNameGap
				    + maxNameWidth
				    + nameToValueGap
				    + maxValueWidth;

				naturalContentWidth =
				    std::max(naturalContentWidth,
				             timeWidth);

				int tooltipWidth =
				    naturalContentWidth + padding * 2;

				//
				// Allow the tooltip to use most of the widget width.
				// This prevents numeric values from being truncated.
				//
				const int maxTooltipWidth =
				    std::max(260, width() - 24);

				tooltipWidth =
				    std::min(tooltipWidth, maxTooltipWidth);

				const int availableContentWidth =
				    tooltipWidth - padding * 2;

				const int fixedContentWidth =
				    colorMarkerWidth
				    + markerToNameGap
				    + nameToValueGap
				    + maxValueWidth;

				int nameColumnWidth =
				    availableContentWidth - fixedContentWidth;

				nameColumnWidth =
				    std::max(60, nameColumnWidth);

				const int lineHeight =
				    tooltipMetrics.height();

				const int tooltipHeight =
				    padding * 2
				    + lineHeight
				    + rowGap
				    + displayRows.size() * lineHeight;

				QPoint tooltipTopLeft =
				    mMousePosition + QPoint(14, 14);

				//
				// Keep tooltip inside the widget, not only inside plotRect.
				//
				if (tooltipTopLeft.x() + tooltipWidth > width() - 8)
				{
				    tooltipTopLeft.setX(
				        mMousePosition.x() - tooltipWidth - 14);
				}

				if (tooltipTopLeft.y() + tooltipHeight > height() - 8)
				{
				    tooltipTopLeft.setY(
				        mMousePosition.y() - tooltipHeight - 14);
				}

				if (tooltipTopLeft.x() < 8)
				{
				    tooltipTopLeft.setX(8);
				}

				if (tooltipTopLeft.y() < 8)
				{
				    tooltipTopLeft.setY(8);
				}

				QRect tooltipRect(tooltipTopLeft,
				                  QSize(tooltipWidth, tooltipHeight));

				painter.setPen(QPen(palette().mid().color(), 1));
				painter.setBrush(palette().base());

				painter.drawRect(tooltipRect);

				painter.setPen(QPen(palette().text().color(), 1));

				int textY =
				    tooltipRect.top() + padding + tooltipMetrics.ascent();

				painter.drawText(tooltipRect.left() + padding,
				                 textY,
				                 timeLine);

				textY += lineHeight + rowGap;

				const int markerX =
				    tooltipRect.left() + padding;

				const int nameX =
				    markerX + colorMarkerWidth + markerToNameGap;

				const int valueX =
				    nameX + nameColumnWidth + nameToValueGap;

				for (const HoverDisplayRow& row : displayRows)
				{
				    const int markerY =
				        textY - tooltipMetrics.ascent() / 2;

				    painter.setPen(QPen(row.mColor, 2));
				    painter.drawLine(markerX,
				                     markerY,
				                     markerX + colorMarkerWidth,
				                     markerY);

				    painter.setPen(QPen(palette().text().color(), 1));

				    const QString displayName =
				        tooltipMetrics.elidedText(row.mName,
				                                  Qt::ElideRight,
				                                  nameColumnWidth);

				    painter.drawText(nameX,
				                     textY,
				                     displayName);

				    //
				    // Do not elide numeric value.
				    //
				    painter.drawText(valueX,
				                     textY,
				                     row.mValue);

				    textY += lineHeight;
				}

				painter.setFont(originalFont);
			}
		}
	}


    //
    // Legend.
    //
    const int legendX = plotRect.right() + 20;
    int legendY = plotRect.top() + 10;

    for (int seriesIndex = 0;
         seriesIndex < validSeries.size();
         ++seriesIndex)
    {
        const PlotSeries& series = validSeries[seriesIndex];

		const QColor color =
		    series.mColor.isValid()
		        ? series.mColor
		        : seriesColor(seriesIndex);
        painter.setPen(QPen(color, 2));

        painter.drawLine(legendX,
                         legendY + 6,
                         legendX + 24,
                         legendY + 6);

        painter.setPen(QPen(palette().text().color(), 1));

        const QString legendText =
		    metrics.elidedText(series.mName,
		                       Qt::ElideRight,
		                       width() - legendX - 40);

		painter.drawText(legendX + 32,
		                 legendY + 11,
		                 legendText);

        legendY += 22;
    }
}

void PlotWidget::mouseMoveEvent(QMouseEvent* event)
{
    mMousePosition = event->position().toPoint();
    mHasMousePosition = true;

    if (mIsPanning)
    {
        const QRect plotRect = plotAreaRect();

        if (plotRect.width() <= 0)
        {
            event->ignore();
            return;
        }

        double dataMinX = 0.0;
        double dataMaxX = 0.0;

        if (!dataXRange(dataMinX, dataMaxX))
        {
            event->ignore();
            return;
        }

        const double visibleRange =
            mPanStartMaxX - mPanStartMinX;

        if (visibleRange <= 0.0)
        {
            event->ignore();
            return;
        }

        const int dxPixels =
            event->position().toPoint().x()
            - mPanStartMousePosition.x();

        /*
         * Drag right means move earlier in time.
         * Drag left means move later in time.
         */
        const double dxData =
            static_cast<double>(dxPixels)
            / static_cast<double>(plotRect.width())
            * visibleRange;

        double newMinX =
            mPanStartMinX - dxData;

        double newMaxX =
            mPanStartMaxX - dxData;

        if (newMinX < dataMinX)
        {
            newMinX = dataMinX;
            newMaxX = dataMinX + visibleRange;
        }

        if (newMaxX > dataMaxX)
        {
            newMaxX = dataMaxX;
            newMinX = dataMaxX - visibleRange;
        }

        mHasCustomXRange = true;
        mCustomMinX = newMinX;
        mCustomMaxX = newMaxX;

        update();

        event->accept();
        return;
    }

    update();

    event->accept();
}

void PlotWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton &&
        mIsPanning)
    {
        mIsPanning = false;
        unsetCursor();

        update();

        event->accept();
        return;
    }

    event->ignore();
}

void PlotWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);

    mHasMousePosition = false;

    if (!mIsPanning)
    {
        unsetCursor();
    }

    update();
}

QRect PlotWidget::plotAreaRect() const
{
    /*
     * Use smaller margins when the plot widget is narrow.
     * This prevents 4-plot layout from collapsing the graph into
     * a thin vertical strip.
     */
    const int widgetWidth =
        width();

    const int widgetHeight =
        height();

    int leftMargin = 85;
    int rightMargin = 190;
    int topMargin = 45;
    int bottomMargin = 75;

    if (widgetWidth < 500)
    {
        leftMargin = 60;
        rightMargin = 35;
    }
    else if (widgetWidth < 700)
    {
        leftMargin = 70;
        rightMargin = 80;
    }

    if (widgetHeight < 320)
    {
        topMargin = 35;
        bottomMargin = 60;
    }

    const int plotWidth =
        widgetWidth - leftMargin - rightMargin;

    const int plotHeight =
        widgetHeight - topMargin - bottomMargin;

    if (plotWidth <= 20 || plotHeight <= 20)
    {
        return QRect();
    }

    return QRect(leftMargin,
                 topMargin,
                 plotWidth,
                 plotHeight);
}

bool PlotWidget::dataXRange(double& minX,
                            double& maxX) const
{
    bool firstValue = true;

    for (const PlotSeries& series : mSeries)
    {
        if (series.mXValues.isEmpty())
        {
            continue;
        }

        auto minMaxX =
            std::minmax_element(series.mXValues.begin(),
                                series.mXValues.end());

        if (firstValue)
        {
            minX = *minMaxX.first;
            maxX = *minMaxX.second;
            firstValue = false;
        }
        else
        {
            minX = std::min(minX, *minMaxX.first);
            maxX = std::max(maxX, *minMaxX.second);
        }
    }

    return !firstValue;
}

void PlotWidget::resetZoom()
{
    mHasCustomXRange = false;
    mCustomMinX = 0.0;
    mCustomMaxX = 0.0;

    mIsPanning = false;
    unsetCursor();

    update();
}

void PlotWidget::wheelEvent(QWheelEvent* event)
{
    const QRect plotRect = plotAreaRect();

    if (!plotRect.contains(event->position().toPoint()))
    {
        event->ignore();
        return;
    }

    double dataMinX = 0.0;
    double dataMaxX = 0.0;

    if (!dataXRange(dataMinX, dataMaxX))
    {
        event->ignore();
        return;
    }

    if (dataMinX == dataMaxX)
    {
        event->ignore();
        return;
    }

    double currentMinX =
        mHasCustomXRange ? mCustomMinX : dataMinX;

    double currentMaxX =
        mHasCustomXRange ? mCustomMaxX : dataMaxX;

    const double currentRange =
        currentMaxX - currentMinX;

    if (currentRange <= 0.0)
    {
        event->ignore();
        return;
    }

    const double mouseRatio =
        static_cast<double>(event->position().x() - plotRect.left())
        / static_cast<double>(plotRect.width());

    const double mouseDataX =
        currentMinX + mouseRatio * currentRange;

    const bool zoomIn =
        event->angleDelta().y() > 0;

    const double zoomFactor =
        zoomIn ? 0.80 : 1.25;

    double newRange =
        currentRange * zoomFactor;

    const double fullRange =
        dataMaxX - dataMinX;

    const double minAllowedRange =
        fullRange * 0.02;

    if (newRange < minAllowedRange)
    {
        newRange = minAllowedRange;
    }

    if (newRange > fullRange)
    {
        newRange = fullRange;
    }

    double newMinX =
        mouseDataX - mouseRatio * newRange;

    double newMaxX =
        newMinX + newRange;

    if (newMinX < dataMinX)
    {
        newMinX = dataMinX;
        newMaxX = dataMinX + newRange;
    }

    if (newMaxX > dataMaxX)
    {
        newMaxX = dataMaxX;
        newMinX = dataMaxX - newRange;
    }

    if (newMinX <= dataMinX &&
        newMaxX >= dataMaxX)
    {
        mHasCustomXRange = false;
        mCustomMinX = 0.0;
        mCustomMaxX = 0.0;
    }
    else
    {
        mHasCustomXRange = true;
        mCustomMinX = newMinX;
        mCustomMaxX = newMaxX;
    }

    update();

    event->accept();
}

void PlotWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    const QRect plotRect = plotAreaRect();

    if (plotRect.contains(event->pos()))
    {
        resetZoom();
        event->accept();
        return;
    }

    event->ignore();
}

void PlotWidget::mousePressEvent(QMouseEvent* event)
{
    const QRect plotRect = plotAreaRect();

    if (event->button() != Qt::LeftButton ||
        !plotRect.contains(event->position().toPoint()))
    {
        event->ignore();
        return;
    }

    /*
     * Pan only makes sense after zoom.
     * If full range is visible, there is nowhere useful to pan.
     */
    if (!mHasCustomXRange)
    {
        event->ignore();
        return;
    }

    double dataMinX = 0.0;
    double dataMaxX = 0.0;

    if (!dataXRange(dataMinX, dataMaxX))
    {
        event->ignore();
        return;
    }

    if (mCustomMaxX <= mCustomMinX)
    {
        event->ignore();
        return;
    }

    mIsPanning = true;
    mPanStartMousePosition = event->position().toPoint();
    mPanStartMinX = mCustomMinX;
    mPanStartMaxX = mCustomMaxX;

    setCursor(Qt::ClosedHandCursor);

    event->accept();
}

