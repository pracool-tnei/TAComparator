#include "ItfParser.h"

#include "DiscoveryParser.h"
#include "PopulationParser.h"

#include "../model/Study.h"

#include <QDebug>

bool ItfParser::parse(const QString& fileName, Study& study)
{
    study.mFileName = fileName;

    DiscoveryParser discoveryParser;

    if (!discoveryParser.parse(fileName, study))
    {
        qDebug() << "Discovery Parser failed.";
        return false;
    }

    PopulationParser populationParser;

    if (!populationParser.parse(fileName, study))
    {
        qDebug() << "Population Parser failed.";
        return false;
    }

    qDebug() << "Parsing completed successfully.";

    return true;
}