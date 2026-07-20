#include "PlotDockWidget.h"

#include <QCloseEvent>

PlotDockWidget::PlotDockWidget(const QString& title,
                               QWidget* parent)
    : QDockWidget(title, parent)
{
}

void PlotDockWidget::closeEvent(QCloseEvent* event)
{
    event->ignore();

    emit closeRequested(this);
}
