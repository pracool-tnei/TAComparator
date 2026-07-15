#include "PlotWidget.h"

#include <QPainter>
#include <QPen>
#include <QFontMetrics>
#include <QPolygonF>
#include <QColor>

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
            QColor(120, 80, 170)
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
}

PlotWidget::PlotWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(350);
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

    update();
}

void PlotWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), palette().window());

    const int leftMargin = 85;
    const int rightMargin = 140;
    const int topMargin = 45;
    const int bottomMargin = 75;

    QRect plotRect(leftMargin,
                   topMargin,
                   width() - leftMargin - rightMargin,
                   height() - topMargin - bottomMargin);

    if (plotRect.width() <= 0 || plotRect.height() <= 0)
        return;

    painter.setPen(QPen(palette().text().color(), 1));

    if (!mTitle.isEmpty())
    {
        painter.drawText(QRect(0, 5, width(), 25),
                         Qt::AlignCenter,
                         mTitle);
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

    painter.setPen(QPen(palette().text().color(), 1));

    for (int i = 0; i <= tickCount; ++i)
    {
        const double ratio =
            static_cast<double>(i) / tickCount;

        const int x =
            plotRect.left()
            + static_cast<int>(ratio * plotRect.width());

        const double value =
    		axisMinX + ratio * (axisMaxX - axisMinX);

        painter.drawLine(x,
                         plotRect.bottom(),
                         x,
                         plotRect.bottom() + tickSize);

        const QString text = formatAxisValue(value);
        const int textWidth = metrics.horizontalAdvance(text);

        painter.drawText(x - textWidth / 2,
                         plotRect.bottom() + tickSize + metrics.height(),
                         text);
    }

    for (int i = 0; i <= tickCount; ++i)
    {
        const double ratio =
            static_cast<double>(i) / tickCount;

        const int y =
            plotRect.bottom()
            - static_cast<int>(ratio * plotRect.height());

        const double value =
            minY + ratio * (maxY - minY);

        painter.drawLine(plotRect.left() - tickSize,
                         y,
                         plotRect.left(),
                         y);

        const QString text = formatAxisValue(value);
        const int textWidth = metrics.horizontalAdvance(text);

        painter.drawText(plotRect.left() - tickSize - textWidth - 6,
                         y + metrics.height() / 3,
                         text);
    }

    if (!mXAxisLabel.isEmpty())
    {
        painter.drawText(QRect(plotRect.left(),
                               height() - 30,
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

	        painter.setPen(QPen(seriesColor(seriesIndex), 2));
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

	            painter.setPen(QPen(seriesColor(seriesIndex), 1));
	            painter.setBrush(seriesColor(seriesIndex));

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
    // Legend.
    //
    const int legendX = plotRect.right() + 20;
    int legendY = plotRect.top() + 10;

    for (int seriesIndex = 0;
         seriesIndex < validSeries.size();
         ++seriesIndex)
    {
        const PlotSeries& series = validSeries[seriesIndex];

        painter.setPen(QPen(seriesColor(seriesIndex), 2));

        painter.drawLine(legendX,
                         legendY + 6,
                         legendX + 24,
                         legendY + 6);

        painter.setPen(QPen(palette().text().color(), 1));

        painter.drawText(legendX + 32,
                         legendY + 11,
                         series.mName);

        legendY += 22;
    }
}
