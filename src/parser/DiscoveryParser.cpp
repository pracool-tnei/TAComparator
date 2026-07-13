#include "DiscoveryParser.h"

#include "../model/Study.h"
#include "../reader/TextFileReader.h"
#include "../model/Component.h"

#include <QDebug>
#include <QFileInfo>

#include <QRegularExpression>

bool DiscoveryParser::parse(const QString& fileName, Study& study)
{
    qDebug() << "========================================";
    qDebug() << "Discovery Pass Started";
    qDebug() << "File :" << fileName;
    qDebug() << "========================================";

    TextFileReader reader;

    if (!reader.open(fileName))
    {
        qDebug() << "ERROR : Unable to open file.";
        return false;
    }

    mStudy = &study;

    QString line;

    //
    // Read Header
    //
    while (reader.readNextLine(line))
    {
        line = line.trimmed();

        if (line.isEmpty())
            continue;

        if (isHeaderLine(line))
        {
            qDebug() << "Found Header";
            break;
        }
    }

    //
    // Read Define blocks
    //
    while (reader.readNextLine(line))
    {
        line = line.trimmed();

        if (line.isEmpty())
            continue;

        if (isDefineLine(line))
        {
            parseDefineBlock(reader, line);
            continue;
        }

        //
        // Define section has finished.
        // Next useful block is Compound SystemMap.
        //
        if (line.startsWith("Compound"))
        {
            parseSystemMap(reader);
            break;
        }
    }

    mStudy->dump();
    return true;
}

void DiscoveryParser::parseDefineBlock(TextFileReader& reader,
                                       const QString& firstLine)
{
    // Example:
    // Define Transient Busbar {

    QString line = firstLine;

    line.remove("Define");
    line.remove("Transient");
    line.remove("{");

    mCurrentObject = line.trimmed();
    mCurrentFields.clear();

    // Read field lines until '}'
    while (reader.readNextLine(line))
    {
        line = line.trimmed();

        if (line == "}")
            break;

        parseFieldLine(line);
    }

    mStudy->addObjectGroup(mCurrentObject);

    ObjectGroup* group = mStudy->objectGroup(mCurrentObject);

    if (!group)
        return;

    for (const QString& field : mCurrentFields)
	{
	    group->addSignal(field);
	}
}

void DiscoveryParser::parseSystemMap(TextFileReader& reader)
{
    QString line;

    while (reader.readNextLine(line))
    {
        line = line.trimmed();

        if (!isRecordLine(line))
            continue;

        QString objectType = line;

        objectType.remove("Record");
        objectType.remove("Transient");
        objectType.remove("{");

        parseRecord(reader, objectType.trimmed());
    }
}

void DiscoveryParser::parseRecord(TextFileReader& reader,
                                  const QString& objectType)
{
    QString line;

    while (reader.readNextLine(line))
    {
        line = line.trimmed();

        if (line == "}")
            return;

        if (line.isEmpty())
            continue;

        parseSystemMapRecord(objectType, line);
    }
}

void DiscoveryParser::parseSystemMapRecord(const QString& objectType,
                                           const QString& line)
{
    QRegularExpression regex(
        "^\\s*(\\d+)\\s+\"([^\"]*)\"\\s+\"([^\"]*)\"");

    QRegularExpressionMatch match = regex.match(line);

    if (!match.hasMatch())
        return;

    int id = match.captured(1).toInt();

    QString name = match.captured(2).trimmed();
    QString qualName = match.captured(3).trimmed();

    QString displayName = name.isEmpty() ? qualName : name;

    ObjectGroup* group = mStudy->objectGroup(objectType);

    if (!group)
        return;

    Component component;
    component.mId = id;
    component.mName = displayName;

    // Copy signal definitions from ObjectGroup
    for (const Signal &signal : group->mSignals)
    {
        component.mSignals.insert(signal.mName, signal);
    }

    group->mComponents.insert(id, component);
}

void DiscoveryParser::parseFieldLine(const QString& line)
{
    QStringList fields = line.split(',', Qt::SkipEmptyParts);

    for (QString field : fields)
    {
        field = field.trimmed();

        QStringList tokens = field.split(':', Qt::SkipEmptyParts);

        if (tokens.isEmpty())
            continue;

        QString fieldName = tokens[0].trimmed();

        mCurrentFields.append(fieldName);
    }
}


bool DiscoveryParser::isHeaderLine(const QString& line) const
{
    return line.startsWith("Header");
}

bool DiscoveryParser::isDefineLine(const QString& line) const
{
    return line.startsWith("Define");
}

bool DiscoveryParser::isRecordLine(const QString& line) const
{
    return line.startsWith("Record");
}