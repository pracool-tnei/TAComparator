#pragma once

#include <QDockWidget>

class QCloseEvent;

class PlotDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit PlotDockWidget(const QString& title,
                            QWidget* parent = nullptr);

signals:
    void closeRequested(PlotDockWidget* dock);

protected:
    void closeEvent(QCloseEvent* event) override;
};
