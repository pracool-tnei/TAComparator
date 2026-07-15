#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include "ObjectGroup.h"

class Study
{
public:

    void setFileName(const QString& fileName);
    QString getFileName() const;

    void addObjectGroup(const QString& name);

    ObjectGroup* getObjectGroup(const QString& name);
    const ObjectGroup* getObjectGroup(const QString& name) const;

    QStringList getObjectGroupNames() const;

    QList<int> getComponentIds(const QString& groupName) const;

    QString getComponentName(const QString& groupName,
                             int componentId) const;

    QStringList getSignalNames(const QString& groupName,
                               int componentId) const;

    QVector<double> getSignalValues(const QString& groupName,
                                    int componentId,
                                    const QString& signalName) const;
	QStringList getStudyTypeNames() const;

	QString getTargetGroupForStudyType(const QString& studyType) const;

	QStringList getSignalSchemaNames(const QString& groupName,
	                                 bool includeKeyField = false) const;

	bool hasPopulatedSignalsForStudyType(const QString& studyType) const;
	

    void clearTimeValues();
    void addTimeValue(double time);
    QVector<double> getTimeValues() const;

    void dump() const;

private:

    QString mFileName;

    QVector<double> mTimeValues;

    QMap<QString, ObjectGroup> mObjectGroups;
};
