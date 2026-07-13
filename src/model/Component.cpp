#pragma once

#include <QString>
#include <QMap>

#include "Signal.h"

class Component
{
public:

    Component() = default;

    Component(int id, const QString& name)
        : mId(id),
        mName(name)
    {
    }

    int mId = 0;

    QString mName;

    QMap<QString, Signal> mSignals;
};