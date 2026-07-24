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

	Qt::PenStyle penStyleForPlotType(PlotType plotType)
	{
		switch (plotType)
		{
		case PlotType::DashedLine:
			return Qt::DashLine;
	
		case PlotType::DottedLine:
			return Qt::DotLine;
	
		case PlotType::DashDotLine:
			return Qt::DashDotLine;
	
		case PlotType::Line:
		case PlotType::Bar:
		default:
			return Qt::SolidLine;
		}
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
	mIsSelectingZoomArea = false;
	
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
	mIsSelectingZoomArea = false;
	
	unsetCursor();
	
	update();

}

void PlotWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), Qt::white);

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
	if (mShowMinorGrid)
	{
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
		if (mShowMajorGrid  && x > plotRect.left() &&
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
		if (mShowMajorGrid && y > plotRect.top() &&
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

	if (mPlotType != PlotType::Bar)
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
	
			QPen linePen(color, thickness);
			linePen.setStyle(penStyleForPlotType(mPlotType));
			linePen.setCapStyle(Qt::RoundCap);
			linePen.setJoinStyle(Qt::RoundJoin);
	
			painter.setPen(linePen);
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
			// For every plotted series, show a value only if nearestTime is inside
			// that series' actual time span.
			//
			// Example:
			//	 File A: 0s to 1s
			//	 File B: 0s to 2s
			//
			// If hover time is 1.5s, File A must not show its last 1s value.
			//
			for (int seriesIndex = 0;
				 seriesIndex < validSeries.size();
				 ++seriesIndex)
			{
				const PlotSeries& series =
					validSeries[seriesIndex];
			
				if (series.mXValues.isEmpty() ||
					series.mYValues.isEmpty() ||
					series.mXValues.size() != series.mYValues.size())
				{
					continue;
				}
			
				const auto minMaxTime =
					std::minmax_element(series.mXValues.begin(),
										series.mXValues.end());
			
				const double seriesMinTime =
					*minMaxTime.first;
			
				const double seriesMaxTime =
					*minMaxTime.second;
			
				const double tolerance =
					std::max(1e-9,
							 (seriesMaxTime - seriesMinTime) * 1e-9);
			
				if (nearestTime < seriesMinTime - tolerance ||
					nearestTime > seriesMaxTime + tolerance)
				{
					continue;
				}
			
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
	// Legend drawn inside the plot area.
	//
	if (!validSeries.isEmpty())
	{
		const int legendPadding = 8;
		const int legendRowHeight = 20;
		const int legendLineWidth = 24;
		const int legendTextGap = 8;
		const int legendOuterMargin = 10;
	
		const int maxLegendTextWidth =
			qMax(100,
				 qMin(260,
					  plotRect.width() / 3));
	
		QVector<QString> legendTexts;
		int actualTextWidth = 0;
	
		for (const PlotSeries& series : validSeries)
		{
			const QString legendText =
				metrics.elidedText(series.mName,
								   Qt::ElideRight,
								   maxLegendTextWidth);
	
			legendTexts.append(legendText);
	
			actualTextWidth =
				qMax(actualTextWidth,
					 metrics.horizontalAdvance(legendText));
		}
	
		const int legendBoxWidth =
			legendPadding * 2 +
			legendLineWidth +
			legendTextGap +
			actualTextWidth;
	
		const int legendBoxHeight =
			legendPadding * 2 +
			legendRowHeight * validSeries.size();
	
		QRect legendRect(plotRect.right() - legendBoxWidth - legendOuterMargin,
						 plotRect.top() + legendOuterMargin,
						 legendBoxWidth,
						 legendBoxHeight);
	
		if (legendRect.left() < plotRect.left() + 4)
		{
			legendRect.moveLeft(plotRect.left() + 4);
		}
	
		if (legendRect.bottom() > plotRect.bottom() - 4)
		{
			legendRect.moveBottom(plotRect.bottom() - 4);
		}
	
		painter.save();
	
		painter.setPen(QPen(QColor(120, 120, 120), 1));
		painter.setBrush(QColor(255, 255, 255, 220));
		painter.drawRoundedRect(legendRect, 5, 5);
	
		int rowY =
			legendRect.top() + legendPadding;
	
		for (int seriesIndex = 0;
			 seriesIndex < validSeries.size();
			 ++seriesIndex)
		{
			const PlotSeries& series =
				validSeries[seriesIndex];
	
			const QColor color =
				series.mColor.isValid()
					? series.mColor
					: seriesColor(seriesIndex);
	
			const int lineY =
				rowY + legendRowHeight / 2;
	
			const int lineStartX =
				legendRect.left() + legendPadding;
	
			const int lineEndX =
				lineStartX + legendLineWidth;
	
			QPen legendPen(color, 2);
			legendPen.setStyle(penStyleForPlotType(mPlotType));
			legendPen.setCapStyle(Qt::RoundCap);
			
			painter.setPen(legendPen);

	
			painter.drawLine(lineStartX,
							 lineY,
							 lineEndX,
							 lineY);
	
			const QRect textRect(lineEndX + legendTextGap,
								 rowY,
								 actualTextWidth,
								 legendRowHeight);
	
			painter.setPen(QPen(palette().text().color(), 1));
	
			painter.drawText(textRect,
							 Qt::AlignLeft | Qt::AlignVCenter,
							 legendTexts.at(seriesIndex));
	
			rowY += legendRowHeight;
		}
	
		painter.restore();
	}

	//
	// Select-to-zoom rectangle.
	//
	if (mIsSelectingZoomArea)
	{
		QRect selectionRect(mZoomSelectionStart,
							mZoomSelectionEnd);
	
		selectionRect =
			selectionRect.normalized()
				.intersected(plotRect);
	
		if (selectionRect.width() >= 2 &&
			selectionRect.height() >= 2)
		{
			painter.save();
	
			QColor fillColor =
				palette().highlight().color();
	
			fillColor.setAlpha(45);
	
			QColor borderColor =
				palette().highlight().color();
	
			borderColor.setAlpha(190);
	
			painter.setPen(QPen(borderColor,
								1,
								Qt::DashLine));
	
			painter.setBrush(fillColor);
	
			painter.drawRect(selectionRect);
	
			painter.restore();
		}
	}
	
}

void PlotWidget::mouseMoveEvent(QMouseEvent* event)
{
    mMousePosition = event->position().toPoint();
    mHasMousePosition = true;

	if (mIsSelectingZoomArea)
	{
		const QRect plotRect =
			plotAreaRect();
	
		const QPoint mousePosition =
			event->position().toPoint();
	
		const int clampedX =
			std::max(plotRect.left(),
					 std::min(mousePosition.x(),
							  plotRect.right()));
	
		const int clampedY =
			std::max(plotRect.top(),
					 std::min(mousePosition.y(),
							  plotRect.bottom()));
	
		mZoomSelectionEnd =
			QPoint(clampedX,
				   clampedY);
	
		update();
	
		event->accept();
		return;
	}

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

		emit xRangeChanged(mCustomMinX,
                   mCustomMaxX,
                   true);

        event->accept();
        return;
    }

    update();

    event->accept();
}

void PlotWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton &&
        mIsSelectingZoomArea)
    {
        const QRect plotRect =
            plotAreaRect();

        mIsSelectingZoomArea = false;

        const int selectionLeftX =
            std::max(plotRect.left(),
                     std::min(mZoomSelectionStart.x(),
                              mZoomSelectionEnd.x()));

        const int selectionRightX =
            std::min(plotRect.right(),
                     std::max(mZoomSelectionStart.x(),
                              mZoomSelectionEnd.x()));

        const int selectionWidth =
            selectionRightX - selectionLeftX;

        /*
         * Ignore accidental clicks or tiny drags.
         */
        if (selectionWidth < 10 ||
            plotRect.width() <= 0)
        {
            unsetCursor();
            update();

            event->accept();
            return;
        }

        double dataMinX = 0.0;
        double dataMaxX = 0.0;

        if (!dataXRange(dataMinX,
                        dataMaxX))
        {
            unsetCursor();
            update();

            event->accept();
            return;
        }

        const double currentMinX =
            mHasCustomXRange
                ? mCustomMinX
                : dataMinX;

        const double currentMaxX =
            mHasCustomXRange
                ? mCustomMaxX
                : dataMaxX;

        const double currentRange =
            currentMaxX - currentMinX;

        const double fullRange =
            dataMaxX - dataMinX;

        if (currentRange <= 0.0 ||
            fullRange <= 0.0)
        {
            unsetCursor();
            update();

            event->accept();
            return;
        }

        const double leftRatio =
            static_cast<double>(selectionLeftX - plotRect.left())
            / static_cast<double>(plotRect.width());

        const double rightRatio =
            static_cast<double>(selectionRightX - plotRect.left())
            / static_cast<double>(plotRect.width());

        double newMinX =
            currentMinX + leftRatio * currentRange;

        double newMaxX =
            currentMinX + rightRatio * currentRange;

        if (newMinX > newMaxX)
        {
            std::swap(newMinX,
                      newMaxX);
        }

        const double selectedRange =
            newMaxX - newMinX;

        const double minimumAllowedRange =
            fullRange * 0.001;

        if (selectedRange < minimumAllowedRange)
        {
            unsetCursor();
            update();

            event->accept();
            return;
        }

        if (newMinX < dataMinX)
        {
            newMinX = dataMinX;
        }

        if (newMaxX > dataMaxX)
        {
            newMaxX = dataMaxX;
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

        unsetCursor();

        update();

		emit xRangeChanged(mCustomMinX,
                   mCustomMaxX,
                   mHasCustomXRange);

        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton &&
        mIsPanning)
    {
        mIsPanning = false;

        unsetCursor();

        update();

		emit xRangeChanged(mCustomMinX,
                   mCustomMaxX,
                   mHasCustomXRange);

        event->accept();
        return;
    }

    event->ignore();
}

void PlotWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);

    mHasMousePosition = false;

	if (!mIsPanning &&
		!mIsSelectingZoomArea)
	{
		unsetCursor();
	}

    update();
}

QRect PlotWidget::plotAreaRect() const
{
    /*
     * Margins for title, axis labels and tick labels.
     * Legend is now drawn inside the plot area, so no large right margin
     * is needed anymore.
     */
    const int widgetWidth =
        width();

    const int widgetHeight =
        height();

    int leftMargin = 95;
    int rightMargin = 35;
    int topMargin = 55;
    int bottomMargin = 85;

    if (widgetWidth < 500)
    {
        leftMargin = 60;
        rightMargin = 20;
    }
    else if (widgetWidth < 700)
    {
        leftMargin = 70;
        rightMargin = 25;
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

    if (plotWidth <= 20 ||
        plotHeight <= 20)
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
    mIsSelectingZoomArea = false;

    unsetCursor();

    update();

    emit xRangeChanged(0.0,
                       0.0,
                       false);
}

void PlotWidget::applyExternalXRange(double minX,
                                     double maxX,
                                     bool hasCustomRange)
{
    /*
     * Used by synchronized zoom.
     * Do not emit xRangeChanged() from here, otherwise synced plots
     * can trigger each other in a loop.
     */
    if (!hasCustomRange)
    {
        mHasCustomXRange = false;
        mCustomMinX = 0.0;
        mCustomMaxX = 0.0;

        mIsPanning = false;
        mIsSelectingZoomArea = false;

        unsetCursor();
        update();
        return;
    }

    if (maxX <= minX)
    {
        return;
    }

    double dataMinX = 0.0;
    double dataMaxX = 0.0;

    if (!dataXRange(dataMinX,
                    dataMaxX))
    {
        return;
    }

    /*
     * Clamp the incoming zoom range to this plot's actual data range.
     */
    double clampedMinX =
        std::max(minX,
                 dataMinX);

    double clampedMaxX =
        std::min(maxX,
                 dataMaxX);

    if (clampedMaxX <= clampedMinX)
    {
        return;
    }

    if (clampedMinX <= dataMinX &&
        clampedMaxX >= dataMaxX)
    {
        mHasCustomXRange = false;
        mCustomMinX = 0.0;
        mCustomMaxX = 0.0;
    }
    else
    {
        mHasCustomXRange = true;
        mCustomMinX = clampedMinX;
        mCustomMaxX = clampedMaxX;
    }

    mIsPanning = false;
    mIsSelectingZoomArea = false;

    unsetCursor();
    update();
}

void PlotWidget::setShowMajorGrid(bool show)
{
    if (mShowMajorGrid == show)
    {
        return;
    }

    mShowMajorGrid = show;
    update();
}

void PlotWidget::setShowMinorGrid(bool show)
{
    if (mShowMinorGrid == show)
    {
        return;
    }

    mShowMinorGrid = show;
    update();
}

bool PlotWidget::showMajorGrid() const
{
    return mShowMajorGrid;
}

bool PlotWidget::showMinorGrid() const
{
    return mShowMinorGrid;
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
	
	emit xRangeChanged(mCustomMinX,
					   mCustomMaxX,
					   mHasCustomXRange);
	
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
    if (event->button() != Qt::LeftButton)
    {
        event->ignore();
        return;
    }

    const QPoint mousePosition =
        event->position().toPoint();

    const QRect plotRect =
        plotAreaRect();

    if (!plotRect.contains(mousePosition))
    {
        event->ignore();
        return;
    }

    double dataMinX = 0.0;
    double dataMaxX = 0.0;

    if (!dataXRange(dataMinX,
                    dataMaxX))
    {
        event->ignore();
        return;
    }

    if (dataMinX == dataMaxX)
    {
        event->ignore();
        return;
    }

    /*
     * Normal state:
     *   left-drag selects a zoom range.
     *
     * Already zoomed:
     *   left-drag keeps the existing pan behaviour.
     *
     * Already zoomed + Shift:
     *   Shift-left-drag selects another zoom range.
     */
    const bool forceSelectZoom =
        event->modifiers().testFlag(Qt::ShiftModifier);

    const bool shouldSelectZoom =
        !mHasCustomXRange ||
        forceSelectZoom;

    if (shouldSelectZoom)
    {
        mIsSelectingZoomArea = true;
        mIsPanning = false;

        mZoomSelectionStart =
            mousePosition;

        mZoomSelectionEnd =
            mousePosition;

        mHasMousePosition = false;

        setCursor(Qt::CrossCursor);

        update();

        event->accept();
        return;
    }

    /*
     * Existing pan behaviour for an already zoomed plot.
     */
    mIsPanning = true;
    mIsSelectingZoomArea = false;

    mPanStartMousePosition =
        mousePosition;

    mPanStartMinX =
        mHasCustomXRange
            ? mCustomMinX
            : dataMinX;

    mPanStartMaxX =
        mHasCustomXRange
            ? mCustomMaxX
            : dataMaxX;

    setCursor(Qt::ClosedHandCursor);

    event->accept();
}

