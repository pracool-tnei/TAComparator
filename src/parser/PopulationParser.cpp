#include "PopulationParser.h"

#include <QDebug>

#include "../reader/TextFileReader.h"
#include "../model/Study.h"

bool PopulationParser::parse(const QString& fileName, Study& study)
{
    qDebug() << "========================================";
    qDebug() << "Population Pass Started";
    qDebug() << "File :" << fileName;
    qDebug() << "========================================";
#if 0
    TextFileReader reader;

    if (!reader.open(fileName))
        return false;

    mStudy = &study;

    QString line;

    //
    // Skip until Compound SystemMap
    //
    while (reader.readNextLine(line))
    {
        line = line.trimmed();

        if (line.startsWith("Compound"))
        {
            parseSystemMap(reader);
            break;
        }
    }

    qDebug() << "Population completed.";
#endif
    return true;
}

void PopulationParser::parseSystemMap(TextFileReader& reader)
{
    QString line;

    while (reader.readNextLine(line))
    {
        line = line.trimmed();

        if (line.startsWith("Compound Step"))
        {
            qDebug() << "";
            qDebug() << "Step found";

            parseStep(reader);
        }
    }
}

void PopulationParser::parseStep(TextFileReader& reader)
{
    QString line;

    while (reader.readNextLine(line))
    {
        line = line.trimmed();

        // End of Compound Step
        if (line == "}")
            return;

        if (line.isEmpty())
            continue;

        // Skip step metadata
        if (line.startsWith("DefRec Step"))
        {
            // consume until closing brace
            while (reader.readNextLine(line))
            {
                line = line.trimmed();

                if (line == "}")
                    break;
            }

            continue;
        }

        // Record block
        if (line.startsWith("Record"))
        {
            QString recordName = line;

            recordName.remove("Record");
            recordName.remove("Transient");
            recordName.remove("{");
            recordName = recordName.trimmed();

            qDebug() << "Record:" << recordName;

            parseRecord(reader, recordName);
        }
    }
}

void PopulationParser::parseRecord(TextFileReader& reader,
                                   const QString& recordName)
{
    QStringList lines;
    QString line;

    while (reader.readNextLine(line))
    {
        line = line.trimmed();

        if (line == "}")
            break;

        if (!line.isEmpty())
            lines.append(line);
    }

    qDebug() << "Record:" << recordName;
    //qDebug() << lines;

    for (const QString &line : lines)
    {
        parseDataLine(recordName, line);
    }
}

void PopulationParser::parseDataLine(const QString& recordName,
                                     const QString& line)
{
    QString firstToken = line.section(' ', 0, 0);

    bool isNewComponent = false;

    firstToken.toInt(&isNewComponent);

    if (isNewComponent)
    {
        qDebug() << "New component:" << line;
    }
    else
    {
        qDebug() << "Continuation :" << line;
    }
}