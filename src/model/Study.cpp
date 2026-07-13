#include "Study.h"

#include <QDebug>

void Study::addObjectGroup(const QString& name)
{
    if (mObjectGroups.contains(name))
        return;

    mObjectGroups.insert(name,
                         ObjectGroup(name));
}

ObjectGroup* Study::objectGroup(const QString& name)
{
    auto it = mObjectGroups.find(name);

    if (it == mObjectGroups.end())
        return nullptr;

    return &it.value();
}

void Study::dump() const
{
    qDebug() << "";
    qDebug() << "================ STUDY ================";

    for (const auto& group : mObjectGroups)
    {
        qDebug() << "";
        qDebug() << group.mName;

        qDebug() << "  Signals";

        //
        // Important:
        // Do not iterate over QMap here.
        // QMap sorts alphabetically.
        // We print mSignalOrder because it preserves ITF Define order.
        //
        for (const QString& signalName : group.mSignalOrder)
        {
            qDebug() << "     -" << signalName;
        }

        qDebug() << "  Components";

        for (const auto& component : group.mComponents)
        {
            qDebug().noquote()
            << QString("     [%1] %2")
                    .arg(component.mId)
                    .arg(component.mName);
        }
    }

    qDebug() << "=======================================";
}
