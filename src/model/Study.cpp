#include "Study.h"

#include <QDebug>

void Study::setFileName(const QString& fileName)
{
    mFileName = fileName;
}

QString Study::getFileName() const
{
    return mFileName;
}

void Study::addObjectGroup(const QString& name)
{
    if (mObjectGroups.contains(name))
        return;

    mObjectGroups.insert(name,
                         ObjectGroup(name));
}

ObjectGroup* Study::getObjectGroup(const QString& name)
{
    auto it = mObjectGroups.find(name);

    if (it == mObjectGroups.end())
        return nullptr;

    return &it.value();
}

const ObjectGroup* Study::getObjectGroup(const QString& name) const
{
    auto it = mObjectGroups.constFind(name);

    if (it == mObjectGroups.constEnd())
        return nullptr;

    return &it.value();
}

QStringList Study::getObjectGroupNames() const
{
    QStringList names;

    for (auto it = mObjectGroups.constBegin();
         it != mObjectGroups.constEnd();
         ++it)
    {
        names.append(it.key());
    }

    return names;
}

QList<int> Study::getComponentIds(const QString& groupName) const
{
    QList<int> ids;

    const ObjectGroup* group = getObjectGroup(groupName);

    if (!group)
        return ids;

    for (auto it = group->mComponents.constBegin();
         it != group->mComponents.constEnd();
         ++it)
    {
        ids.append(it.key());
    }

    return ids;
}

QString Study::getComponentName(const QString& groupName,
                                int componentId) const
{
    const ObjectGroup* group = getObjectGroup(groupName);

    if (!group)
        return QString();

    auto componentIt = group->mComponents.constFind(componentId);

    if (componentIt == group->mComponents.constEnd())
        return QString();

    return componentIt.value().mName;
}

QStringList Study::getSignalNames(const QString& groupName,
                                  int componentId) const
{
    QStringList names;

    const ObjectGroup* group = getObjectGroup(groupName);

    if (!group)
        return names;

    auto componentIt = group->mComponents.constFind(componentId);

    if (componentIt == group->mComponents.constEnd())
        return names;

    const Component& component = componentIt.value();

    for (auto signalIt = component.mSignals.constBegin();
         signalIt != component.mSignals.constEnd();
         ++signalIt)
    {
        const Signal& signal = signalIt.value();

        if (!signal.mValues.isEmpty())
        {
            names.append(signal.mName);
        }
    }

    return names;
}

QVector<double> Study::getSignalValues(const QString& groupName,
                                       int componentId,
                                       const QString& signalName) const
{
    const ObjectGroup* group = getObjectGroup(groupName);

    if (!group)
        return {};

    auto componentIt = group->mComponents.constFind(componentId);

    if (componentIt == group->mComponents.constEnd())
        return {};

    const Component& component = componentIt.value();

    auto signalIt = component.mSignals.constFind(signalName);

    if (signalIt == component.mSignals.constEnd())
        return {};

    return signalIt.value().mValues;
}

QStringList Study::getStudyTypeNames() const
{
    QStringList studyTypes;

    for (auto it = mObjectGroups.constBegin();
         it != mObjectGroups.constEnd();
         ++it)
    {
        const QString groupName = it.key();

        //
        // At the moment, study/result types are ITF records ending with Output.
        //
        if (!groupName.endsWith("Output"))
            continue;

        //
        // Only expose output types that actually have populated values.
        //
        if (!hasPopulatedSignalsForStudyType(groupName))
            continue;

        studyTypes.append(groupName);
    }

    return studyTypes;
}

QString Study::getTargetGroupForStudyType(const QString& studyType) const
{
    const ObjectGroup* studyTypeGroup = getObjectGroup(studyType);

    if (!studyTypeGroup)
        return QString();

    if (studyTypeGroup->mSignalOrder.isEmpty())
        return QString();

    //
    // The first field usually identifies the component key.
    //
    // Examples:
    //   BusbarOutput     -> BusID
    //   MonBranchOutput  -> BranchID
    //   GeneratorOutput  -> GenID
    //   AVROutput        -> GenID
    //
    QString keyField = studyTypeGroup->mSignalOrder.first();

    if (!keyField.endsWith("ID"))
        return QString();

    QString candidateGroup = keyField;
    candidateGroup.chop(2); // remove "ID"

    //
    // Small IPSA aliases.
    //
    // These are not study-type hardcodes.
    // They only translate ITF key names to SystemMap group names.
    //
    if (candidateGroup == "Bus")
    {
        candidateGroup = "Busbar";
    }
    else if (candidateGroup == "Gen")
    {
        candidateGroup = "Generator";
    }

    const ObjectGroup* targetGroup = getObjectGroup(candidateGroup);

    if (!targetGroup)
        return QString();

    if (targetGroup->mComponents.isEmpty())
        return QString();

    return candidateGroup;
}

QStringList Study::getSignalSchemaNames(const QString& groupName,
                                        bool includeKeyField) const
{
    QStringList names;

    const ObjectGroup* group = getObjectGroup(groupName);

    if (!group)
        return names;

    const int startIndex = includeKeyField ? 0 : 1;

    for (int i = startIndex; i < group->mSignalOrder.size(); ++i)
    {
        names.append(group->mSignalOrder[i]);
    }

    return names;
}

bool Study::hasPopulatedSignalsForStudyType(const QString& studyType) const
{
    const QString targetGroupName =
        getTargetGroupForStudyType(studyType);

    if (targetGroupName.isEmpty())
        return false;

    const ObjectGroup* targetGroup =
        getObjectGroup(targetGroupName);

    if (!targetGroup)
        return false;

    const QStringList schemaSignals =
        getSignalSchemaNames(studyType, false);

    if (schemaSignals.isEmpty())
        return false;

    for (auto componentIt = targetGroup->mComponents.constBegin();
         componentIt != targetGroup->mComponents.constEnd();
         ++componentIt)
    {
        const Component& component = componentIt.value();

        for (const QString& signalName : schemaSignals)
        {
            auto signalIt = component.mSignals.constFind(signalName);

            if (signalIt == component.mSignals.constEnd())
                continue;

            if (!signalIt.value().mValues.isEmpty())
                return true;
        }
    }

    return false;
}

void Study::clearTimeValues()
{
    mTimeValues.clear();
}

void Study::addTimeValue(double time)
{
    mTimeValues.append(time);
}

QVector<double> Study::getTimeValues() const
{
    return mTimeValues;
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
