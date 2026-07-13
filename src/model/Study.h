#pragma once

#include <QMap>
#include <QString>

#include "ObjectGroup.h"

class Study
{
public:

    void addObjectGroup(const QString& name);

    ObjectGroup* objectGroup(const QString& name);

    void dump() const;

public:

    QString mFileName;

private:

    QMap<QString, ObjectGroup> mObjectGroups;
};