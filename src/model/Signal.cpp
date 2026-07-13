#pragma once

#include <QString>
#include <QVector>

class Signal
{
public:

    Signal() = default;

    explicit Signal(const QString& name)
        : mName(name)
    {
    }

    QString mName;

    QVector<double> mValues;
};