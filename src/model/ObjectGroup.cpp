#include "ObjectGroup.h"

ObjectGroup::ObjectGroup(const QString& name)
    : mName(name)
{
}

void ObjectGroup::addSignal(const QString& signalName)
{
    if (mSignals.contains(signalName))
        return;

    //
    // Preserve original discovery order.
    //
    mSignalOrder.append(signalName);

    //
    // Also keep map for lookup by name.
    //
    mSignals.insert(signalName,
                    Signal(signalName));
}

void ObjectGroup::addComponent(int id,
                               const QString& name)
{
    if (mComponents.contains(id))
        return;

    mComponents.insert(id,
                       Component(id, name));
}
