#pragma once

#include <QString>
#include <QMap>

#include "Component.h"
#include "Signal.h"

class ObjectGroup
{
public:

    ObjectGroup() = default;

    explicit ObjectGroup(const QString& name);

    void addSignal(const QString& signalName);

    void addComponent(int id,
                      const QString& name);

    QString mName;

    //
    // Preserves the exact signal order from the ITF Define block.
    // This is required by PopulationParser to map value[0], value[1], ...
    // to the correct signal.
    //
    QVector<QString> mSignalOrder;

    //
    // Fast lookup by signal name.
    //
    QMap<QString, Signal> mSignals;

    QMap<int, Component> mComponents;
};