#include "PopulationParser.h"

#include <QDebug>
#include <QRegularExpression>

#include "../reader/TextFileReader.h"
#include "../model/Study.h"
#include "../model/ObjectGroup.h"
#include "../model/Component.h"
#include "../model/Signal.h"

namespace
{
    QStringList splitTokens(const QString& line)
    {
        return line.split(QRegularExpression("\\s+"),
                          Qt::SkipEmptyParts);
    }

    QString vectorToString(const QVector<double>& values,
                           int maxValues)
    {
        QStringList textValues;

        if (values.size() <= maxValues)
        {
            for (double value : values)
            {
                textValues.append(QString::number(value, 'g', 12));
            }

            return "[" + textValues.join(", ") + "]";
        }

        const int headCount = maxValues / 2;
        const int tailCount = maxValues - headCount;

        for (int i = 0; i < headCount; ++i)
        {
            textValues.append(QString::number(values[i], 'g', 12));
        }

        textValues.append("...");

        for (int i = values.size() - tailCount; i < values.size(); ++i)
        {
            textValues.append(QString::number(values[i], 'g', 12));
        }

        return "[" + textValues.join(", ") + "]";
    }
}

bool PopulationParser::parse(const QString& fileName, Study& study)
{
    qDebug() << "========================================";
    qDebug() << "Population Pass Started";
    qDebug() << "File :" << fileName;
    qDebug() << "========================================";

    TextFileReader reader;

    if (!reader.open(fileName))
        return false;

   	mStudy = &study;

	mStudy->clearTimeValues();
	mRunStepRowsStored = 0;


	mBusbarOutputRowsStored = 0;

	mMonBranchOutputRowsStored = 0;
	mMonBranchOutputRowsSkipped = 0;

	mGeneratorOutputRowsStored = 0;

	mAVROutputRowsStored = 0;

	mGovernorOutputRowsStored = 0;
	mGovernorOutputRowsSkipped = 0;

	mIndMachOutputRowsStored = 0;

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

    printPopulationSummary();

    qDebug() << "Population completed.";

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
    QStringList physicalLines;
    QString line;

    while (reader.readNextLine(line))
    {
        line = line.trimmed();

        if (line == "}")
            break;

        if (!line.isEmpty())
            physicalLines.append(line);
    }

    if (recordName == "Messages")
        return;

    ObjectGroup* recordGroup = mStudy->getObjectGroup(recordName);

    if (!recordGroup)
    {
        qDebug() << "WARNING: No ObjectGroup found for record:"
                 << recordName;
        return;
    }

    const int expectedFieldCount = recordGroup->mSignalOrder.size();

    if (expectedFieldCount == 0)
    {
        qDebug() << "WARNING: Record has no discovered fields:"
                 << recordName;
        return;
    }

    QStringList currentTokens;
    int logicalRowCount = 0;

    for (const QString& physicalLine : physicalLines)
    {
        currentTokens.append(splitTokens(physicalLine));

        while (currentTokens.size() >= expectedFieldCount)
        {
            QStringList logicalTokens =
                currentTokens.mid(0, expectedFieldCount);

            parseDataLine(recordName, logicalTokens);

            logicalRowCount++;

            currentTokens =
                currentTokens.mid(expectedFieldCount);
        }
    }

    if (!currentTokens.isEmpty())
    {
        qDebug() << "WARNING: Incomplete logical record for"
                 << recordName
                 << "remaining tokens:"
                 << currentTokens.size()
                 << "expected:"
                 << expectedFieldCount
                 << "tokens:"
                 << currentTokens;
    }

    qDebug() << "Record:" << recordName
             << "physical lines:" << physicalLines.size()
             << "logical rows:" << logicalRowCount
             << "expected fields:" << expectedFieldCount;
}
								   
void PopulationParser::parseDataLine(const QString& recordName,
                                     const QStringList& tokens)
{
    if (recordName == "RunStep")
    {
        populateRunStep(tokens);
        return;
    }

    if (recordName == "BusbarOutput")
    {
        populateBusbarOutput(tokens);
        return;
    }

    if (recordName == "MonBranchOutput")
    {
        populateMonBranchOutput(tokens);
        return;
    }

    if (recordName == "GeneratorOutput")
    {
        populateGeneratorOutput(tokens);
        return;
    }

    if (recordName == "AVROutput")
    {
        populateAVROutput(tokens);
        return;
    }
	
	if (recordName == "GovernorOutput")
	{
		populateGovernorOutput(tokens);
		return;
	}

	if (recordName == "IndMachOutput")
	{
	    populateIndMachOutput(tokens);
	    return;
	}

    //
    // Other records are intentionally skipped for now.
    //
}


void PopulationParser::populateBusbarOutput(const QStringList& tokens)
{
    ObjectGroup* recordGroup = mStudy->getObjectGroup("BusbarOutput");
    ObjectGroup* busbarGroup = mStudy->getObjectGroup("Busbar");

    if (!recordGroup || !busbarGroup)
        return;

    if (tokens.size() != recordGroup->mSignalOrder.size())
    {
        qDebug() << "WARNING: BusbarOutput token count mismatch."
                 << "tokens:" << tokens.size()
                 << "expected:" << recordGroup->mSignalOrder.size();
        return;
    }

    bool idOk = false;
    int busId = tokens[0].toInt(&idOk);

    if (!idOk)
    {
        qDebug() << "WARNING: Invalid BusID in BusbarOutput:"
                 << tokens[0];
        return;
    }

    auto componentIt = busbarGroup->mComponents.find(busId);

    if (componentIt == busbarGroup->mComponents.end())
    {
        qDebug() << "WARNING: Busbar component not found for BusID:"
                 << busId;
        return;
    }

    Component& component = componentIt.value();

    //
    // token[0] is BusID, so actual signal values start from index 1.
    //
    for (int i = 1; i < tokens.size(); ++i)
    {
        const QString signalName = recordGroup->mSignalOrder[i];

        bool valueOk = false;
        double value = tokens[i].toDouble(&valueOk);

        if (!valueOk)
        {
            qDebug() << "WARNING: Invalid numeric value in BusbarOutput."
                     << "BusID:" << busId
                     << "Signal:" << signalName
                     << "Value:" << tokens[i];
            continue;
        }

        if (!component.mSignals.contains(signalName))
        {
            component.mSignals.insert(signalName,
                                      Signal(signalName));
        }

        component.mSignals[signalName].mValues.append(value);
    }

    mBusbarOutputRowsStored++;
}

void PopulationParser::populateMonBranchOutput(const QStringList& tokens)
{
    ObjectGroup* recordGroup = mStudy->getObjectGroup("MonBranchOutput");
    ObjectGroup* branchGroup = mStudy->getObjectGroup("Branch");

    if (!recordGroup || !branchGroup)
        return;

    if (tokens.size() != recordGroup->mSignalOrder.size())
    {
        qDebug() << "WARNING: MonBranchOutput token count mismatch."
                 << "tokens:" << tokens.size()
                 << "expected:" << recordGroup->mSignalOrder.size();
        return;
    }

    bool idOk = false;
    int branchId = tokens[0].toInt(&idOk);

    if (!idOk)
    {
        qDebug() << "WARNING: Invalid BranchID in MonBranchOutput:"
                 << tokens[0];
        mMonBranchOutputRowsSkipped++;
        return;
    }

    auto componentIt = branchGroup->mComponents.find(branchId);

    if (componentIt == branchGroup->mComponents.end())
    {
        //
        // In the sample file, BranchID 0 appears as a complete logical record,
        // but it is not present in SystemMap. So we skip it safely.
        //
        mMonBranchOutputRowsSkipped++;
        return;
    }

    Component& component = componentIt.value();

    //
    // token[0] is BranchID, so actual signal values start from index 1.
    //
    for (int i = 1; i < tokens.size(); ++i)
    {
        const QString signalName = recordGroup->mSignalOrder[i];

        bool valueOk = false;
        double value = tokens[i].toDouble(&valueOk);

        if (!valueOk)
        {
            qDebug() << "WARNING: Invalid numeric value in MonBranchOutput."
                     << "BranchID:" << branchId
                     << "Signal:" << signalName
                     << "Value:" << tokens[i];
            continue;
        }

        if (!component.mSignals.contains(signalName))
        {
            component.mSignals.insert(signalName,
                                      Signal(signalName));
        }

        component.mSignals[signalName].mValues.append(value);
    }

    mMonBranchOutputRowsStored++;
}

void PopulationParser::populateGeneratorOutput(const QStringList& tokens)
{
    ObjectGroup* recordGroup = mStudy->getObjectGroup("GeneratorOutput");
    ObjectGroup* generatorGroup = mStudy->getObjectGroup("Generator");

    if (!recordGroup || !generatorGroup)
        return;

    if (tokens.size() != recordGroup->mSignalOrder.size())
    {
        qDebug() << "WARNING: GeneratorOutput token count mismatch."
                 << "tokens:" << tokens.size()
                 << "expected:" << recordGroup->mSignalOrder.size();
        return;
    }

    bool idOk = false;
    int generatorId = tokens[0].toInt(&idOk);

    if (!idOk)
    {
        qDebug() << "WARNING: Invalid GenID in GeneratorOutput:"
                 << tokens[0];
        return;
    }

    auto componentIt = generatorGroup->mComponents.find(generatorId);

    if (componentIt == generatorGroup->mComponents.end())
    {
        qDebug() << "WARNING: Generator component not found for GenID:"
                 << generatorId;
        return;
    }

    Component& component = componentIt.value();

    //
    // token[0] is GenID.
    // Store all remaining GeneratorOutput fields:
    //
    // token[1] -> BusID
    // token[2] -> Angle
    // token[3] -> Poles
    // ...
    //
    for (int i = 1; i < tokens.size(); ++i)
    {
        const QString signalName = recordGroup->mSignalOrder[i];

        bool valueOk = false;
        double value = tokens[i].toDouble(&valueOk);

        if (!valueOk)
        {
            qDebug() << "WARNING: Invalid numeric value in GeneratorOutput."
                     << "GenID:" << generatorId
                     << "Signal:" << signalName
                     << "Value:" << tokens[i];
            continue;
        }

        if (!component.mSignals.contains(signalName))
        {
            component.mSignals.insert(signalName,
                                      Signal(signalName));
        }

        component.mSignals[signalName].mValues.append(value);
    }

    mGeneratorOutputRowsStored++;
}

void PopulationParser::populateAVROutput(const QStringList& tokens)
{
    ObjectGroup* recordGroup = mStudy->getObjectGroup("AVROutput");
    ObjectGroup* generatorGroup = mStudy->getObjectGroup("Generator");

    if (!recordGroup || !generatorGroup)
        return;

    if (tokens.size() != recordGroup->mSignalOrder.size())
    {
        qDebug() << "WARNING: AVROutput token count mismatch."
                 << "tokens:" << tokens.size()
                 << "expected:" << recordGroup->mSignalOrder.size();
        return;
    }

    bool idOk = false;
    int genId = tokens[0].toInt(&idOk);

    if (!idOk)
    {
        qDebug() << "WARNING: Invalid GenID in AVROutput:"
                 << tokens[0];
        return;
    }

    auto componentIt = generatorGroup->mComponents.find(genId);

    if (componentIt == generatorGroup->mComponents.end())
    {
        qDebug() << "WARNING: Generator component not found for AVROutput GenID:"
                 << genId;
        return;
    }

    Component& component = componentIt.value();

    //
    // token[0] is GenID.
    // The actual AVROutput values start from token[1].
    //
    for (int i = 1; i < tokens.size(); ++i)
    {
        const QString signalName = recordGroup->mSignalOrder[i];

        bool valueOk = false;
        double value = tokens[i].toDouble(&valueOk);

        if (!valueOk)
        {
            qDebug() << "WARNING: Invalid numeric value in AVROutput."
                     << "GenID:" << genId
                     << "Signal:" << signalName
                     << "Value:" << tokens[i];
            continue;
        }

        if (!component.mSignals.contains(signalName))
        {
            component.mSignals.insert(signalName,
                                      Signal(signalName));
        }

        component.mSignals[signalName].mValues.append(value);
    }

    mAVROutputRowsStored++;
}


void PopulationParser::populateGovernorOutput(const QStringList& tokens)
{
    ObjectGroup* recordGroup = mStudy->getObjectGroup("GovernorOutput");
    ObjectGroup* generatorGroup = mStudy->getObjectGroup("Generator");

    if (!recordGroup || !generatorGroup)
        return;

    if (tokens.size() != recordGroup->mSignalOrder.size())
    {
        qDebug() << "WARNING: GovernorOutput token count mismatch."
                 << "tokens:" << tokens.size()
                 << "expected:" << recordGroup->mSignalOrder.size();

        mGovernorOutputRowsSkipped++;
        return;
    }

    bool idOk = false;
    int genId = tokens[0].toInt(&idOk);

    if (!idOk)
    {
        qDebug() << "WARNING: Invalid GenID in GovernorOutput:"
                 << tokens[0];

        mGovernorOutputRowsSkipped++;
        return;
    }

    auto componentIt = generatorGroup->mComponents.find(genId);

    if (componentIt == generatorGroup->mComponents.end())
    {
        qDebug() << "WARNING: Generator component not found for GovernorOutput GenID:"
                 << genId;

        mGovernorOutputRowsSkipped++;
        return;
    }

    Component& component = componentIt.value();

    //
    // token[0] is GenID.
    // Actual GovernorOutput values start from token[1].
    //
    for (int i = 1; i < tokens.size(); ++i)
    {
        const QString signalName = recordGroup->mSignalOrder[i];

        bool valueOk = false;
        double value = tokens[i].toDouble(&valueOk);

        if (!valueOk)
        {
            qDebug() << "WARNING: Invalid numeric value in GovernorOutput."
                     << "GenID:" << genId
                     << "Signal:" << signalName
                     << "Value:" << tokens[i];

            continue;
        }

        if (!component.mSignals.contains(signalName))
        {
            component.mSignals.insert(signalName,
                                      Signal(signalName));
        }

        component.mSignals[signalName].mValues.append(value);
    }

    mGovernorOutputRowsStored++;
}

void PopulationParser::populateIndMachOutput(const QStringList& tokens)
{
    ObjectGroup* recordGroup = mStudy->getObjectGroup("IndMachOutput");
    ObjectGroup* indMachGroup = mStudy->getObjectGroup("IndMach");

    if (!recordGroup || !indMachGroup)
        return;

    if (tokens.size() != recordGroup->mSignalOrder.size())
    {
        qDebug() << "WARNING: IndMachOutput token count mismatch."
                 << "tokens:" << tokens.size()
                 << "expected:" << recordGroup->mSignalOrder.size();
        return;
    }

    bool idOk = false;
    int indMachId = tokens[0].toInt(&idOk);

    if (!idOk)
    {
        qDebug() << "WARNING: Invalid IMID in IndMachOutput:"
                 << tokens[0];
        return;
    }

    auto componentIt = indMachGroup->mComponents.find(indMachId);

    if (componentIt == indMachGroup->mComponents.end())
    {
        qDebug() << "WARNING: IndMach component not found for IMID:"
                 << indMachId;
        return;
    }

    Component& component = componentIt.value();

    for (int i = 1; i < tokens.size(); ++i)
    {
        const QString signalName = recordGroup->mSignalOrder[i];

        bool valueOk = false;
        double value = tokens[i].toDouble(&valueOk);

        if (!valueOk)
        {
            qDebug() << "WARNING: Invalid numeric value in IndMachOutput."
                     << "IMID:" << indMachId
                     << "Signal:" << signalName
                     << "Value:" << tokens[i];
            continue;
        }

        if (!component.mSignals.contains(signalName))
        {
            component.mSignals.insert(signalName,
                                      Signal(signalName));
        }

        component.mSignals[signalName].mValues.append(value);
    }

    mIndMachOutputRowsStored++;
}

void PopulationParser::populateRunStep(const QStringList& tokens)
{
    ObjectGroup* recordGroup = mStudy->getObjectGroup("RunStep");

    if (!recordGroup)
        return;

    if (tokens.size() != recordGroup->mSignalOrder.size())
    {
        qDebug() << "WARNING: RunStep token count mismatch."
                 << "tokens:" << tokens.size()
                 << "expected:" << recordGroup->mSignalOrder.size();
        return;
    }

    const int timeIndex = recordGroup->mSignalOrder.indexOf("Time");

    if (timeIndex < 0)
    {
        qDebug() << "WARNING: RunStep has no Time field.";
        return;
    }

    bool valueOk = false;
    double time = tokens[timeIndex].toDouble(&valueOk);

    if (!valueOk)
    {
        qDebug() << "WARNING: Invalid Time value in RunStep:"
                 << tokens[timeIndex];
        return;
    }

	mStudy->addTimeValue(time);

    mRunStepRowsStored++;
}

void PopulationParser::printPopulationSummaryRunStep() const
{
	qDebug() << "RunStep rows stored:"
	         << mRunStepRowsStored;

	QVector<double> timeValues = mStudy->getTimeValues();

	qDebug() << "Time value count:"
	         << timeValues.size();

	if (!timeValues.isEmpty())
	{
	    qDebug() << "First time:"
	             << timeValues.first();

	    qDebug() << "Last time:"
	             << timeValues.last();

	    qDebug().noquote()
	        << QString("Time values: %1")
	              .arg(vectorToString(timeValues, 40));
	}

	qDebug() << "============================================";
}

void PopulationParser::printPopulationSummaryBusbar() const
{
	qDebug() << "BusbarOutput rows stored:"
             << mBusbarOutputRowsStored;

    ObjectGroup* busbarGroup = mStudy->getObjectGroup("Busbar");

    if (!busbarGroup)
    {
        qDebug() << "Busbar group not found.";
        qDebug() << "============================================";
        return;
    }

    auto componentIt = busbarGroup->mComponents.find(1);

    if (componentIt == busbarGroup->mComponents.end())
    {
        qDebug() << "Busbar component 1 not found.";
        qDebug() << "============================================";
        return;
    }

    const Component& component = componentIt.value();

    auto signalIt = component.mSignals.find("VMag");

    if (signalIt == component.mSignals.end())
    {
        qDebug() << "Busbar 1 VMag signal not found.";
        qDebug() << "============================================";
        return;
    }

    const Signal& signal = signalIt.value();

    qDebug() << "Busbar 1 VMag value count:"
             << signal.mValues.size();

    if (!signal.mValues.isEmpty())
    {
        qDebug() << "Busbar 1 first VMag:"
                 << signal.mValues.first();

        qDebug() << "Busbar 1 last VMag:"
                 << signal.mValues.last();
    }

    qDebug() << "============================================";
}

void PopulationParser::printPopulationSummaryMonBranch() const
{
	qDebug() << "MonBranchOutput rows stored:"
	         << mMonBranchOutputRowsStored;

	qDebug() << "MonBranchOutput rows skipped:"
	         << mMonBranchOutputRowsSkipped;

	ObjectGroup* branchGroup = mStudy->getObjectGroup("Branch");

	if (branchGroup)
	{
	    auto branchIt = branchGroup->mComponents.find(6);

	    if (branchIt != branchGroup->mComponents.end())
	    {
	        const Component& branch = branchIt.value();

	        auto signalIt = branch.mSignals.find("SndVoltage");

	        if (signalIt != branch.mSignals.end())
	        {
	            const Signal& signal = signalIt.value();

	            qDebug() << "Branch 6 SndVoltage value count:"
	                     << signal.mValues.size();

	            if (!signal.mValues.isEmpty())
	            {
	                qDebug() << "Branch 6 first SndVoltage:"
	                         << signal.mValues.first();

	                qDebug() << "Branch 6 last SndVoltage:"
	                         << signal.mValues.last();
	            }
	        }
	        else
	        {
	            qDebug() << "Branch 6 SndVoltage signal not found.";
	        }
	    }
	    else
	    {
	        qDebug() << "Branch component 6 not found.";
	    }
	}
	else
	{
	    qDebug() << "Branch group not found.";
	}

	qDebug() << "============================================";
}

void PopulationParser::printPopulationSummaryGenerator() const
{
	qDebug() << "GeneratorOutput rows stored:"
	         << mGeneratorOutputRowsStored;

	ObjectGroup* generatorGroup = mStudy->getObjectGroup("Generator");

	if (generatorGroup)
	{
	    auto generatorIt = generatorGroup->mComponents.find(1);

	    if (generatorIt != generatorGroup->mComponents.end())
	    {
	        const Component& generator = generatorIt.value();

	        auto signalIt = generator.mSignals.find("Angle");

	        if (signalIt != generator.mSignals.end())
	        {
	            const Signal& signal = signalIt.value();

	            qDebug() << "Generator 1 Angle value count:"
	                     << signal.mValues.size();

	            if (!signal.mValues.isEmpty())
	            {
	                qDebug() << "Generator 1 first Angle:"
	                         << signal.mValues.first();

	                qDebug() << "Generator 1 last Angle:"
	                         << signal.mValues.last();
	            }
	        }
	        else
	        {
	            qDebug() << "Generator 1 Angle signal not found.";
	        }
	    }
	    else
	    {
	        qDebug() << "Generator component 1 not found.";
	    }
	}
	else
	{
	    qDebug() << "Generator group not found.";
	}

	qDebug() << "============================================";
}

void PopulationParser::printPopulationSummaryAVR() const
{
	qDebug() << "AVROutput rows stored:"
	         << mAVROutputRowsStored;

	ObjectGroup* generatorGroup = mStudy->getObjectGroup("Generator");

	if (generatorGroup)
	{
	    auto generatorIt = generatorGroup->mComponents.find(1);

	    if (generatorIt != generatorGroup->mComponents.end())
	    {
	        const Component& generator = generatorIt.value();

	        auto signalIt = generator.mSignals.find("Vt");

	        if (signalIt != generator.mSignals.end())
	        {
	            const Signal& signal = signalIt.value();

	            qDebug() << "Generator 1 AVROutput Vt value count:"
	                     << signal.mValues.size();

	            if (!signal.mValues.isEmpty())
	            {
	                qDebug() << "Generator 1 first AVROutput Vt:"
	                         << signal.mValues.first();

	                qDebug() << "Generator 1 last AVROutput Vt:"
	                         << signal.mValues.last();
	            }
	        }
	        else
	        {
	            qDebug() << "Generator 1 AVROutput Vt signal not found.";
	        }
	    }
	    else
	    {
	        qDebug() << "Generator component 1 not found.";
	    }
	}
	else
	{
	    qDebug() << "Generator group not found.";
	}

	qDebug() << "============================================";
}

void PopulationParser::printPopulationSummaryGovernor() const
{
	qDebug() << "GovernorOutput rows stored:"
	         << mGovernorOutputRowsStored;

	qDebug() << "GovernorOutput rows skipped:"
	         << mGovernorOutputRowsSkipped;

	ObjectGroup* generatorGroupForGovernor = mStudy->getObjectGroup("Generator");

	if (generatorGroupForGovernor)
	{
	    //
	    // From the SystemMap snippet, Governor is associated with GenID 2.
	    // Since GovernorOutput is keyed by GenID, check Generator 2.
	    //
	    auto generatorIt = generatorGroupForGovernor->mComponents.find(2);

	    if (generatorIt != generatorGroupForGovernor->mComponents.end())
	    {
	        const Component& generator = generatorIt.value();

	        auto signalIt = generator.mSignals.find("Pb");

	        if (signalIt != generator.mSignals.end())
	        {
	            const Signal& signal = signalIt.value();

	            qDebug() << "Generator 2 GovernorOutput Pb value count:"
	                     << signal.mValues.size();

	            if (!signal.mValues.isEmpty())
	            {
	                qDebug() << "Generator 2 first GovernorOutput Pb:"
	                         << signal.mValues.first();

	                qDebug() << "Generator 2 last GovernorOutput Pb:"
	                         << signal.mValues.last();
	            }
	        }
	        else
	        {
	            qDebug() << "Generator 2 GovernorOutput Pb signal not found.";
	        }
	    }
	    else
	    {
	        qDebug() << "Generator component 2 not found.";
	    }
	}
	else
	{
	    qDebug() << "Generator group not found.";
	}
	qDebug() << "============================================";
}

void PopulationParser::printPopulationSummary() const
{
    qDebug() << "";
    qDebug() << "============ START POPULATION SUMMARY ============";

	printPopulationSummaryRunStep();

	printPopulationSummaryBusbar();

	printPopulationSummaryMonBranch();

	printPopulationSummaryGenerator();

	printPopulationSummaryAVR();

	printPopulationSummaryGovernor();

#if 0
	qDebug() << "";
	qDebug() << "============ VECTOR CROSS CHECK ============";
	printSignalVector("Busbar", 1, "VMag");
	printSignalVector("Busbar", 1, "VAngle");
	printSignalVector("Branch", 6, "SndVoltage");
	printSignalVector("Branch", 6, "SndRealPow");
	printSignalVector("Generator", 1, "Angle");
	printSignalVector("Generator", 1, "RealPow");
	printSignalVector("Generator", 1, "Vt");
	qDebug() << "============================================";
#endif

	qDebug() << "============ END POPULATION SUMMARY ============";
}

void PopulationParser::printSignalVector(const QString& groupName,
                                         int componentId,
                                         const QString& signalName,
                                         int maxValues) const
{
    ObjectGroup* group = mStudy->getObjectGroup(groupName);

    if (!group)
    {
        qDebug() << "Vector check failed. Group not found:"
                 << groupName;
        return;
    }

    auto componentIt = group->mComponents.find(componentId);

    if (componentIt == group->mComponents.end())
    {
        qDebug() << "Vector check failed. Component not found:"
                 << groupName
                 << componentId;
        return;
    }

    const Component& component = componentIt.value();

    auto signalIt = component.mSignals.find(signalName);

    if (signalIt == component.mSignals.end())
    {
        qDebug() << "Vector check failed. Signal not found:"
                 << groupName
                 << componentId
                 << signalName;
        return;
    }

    const Signal& signal = signalIt.value();

    qDebug().noquote()
        << QString("%1 [%2] %3")
              .arg(groupName)
              .arg(componentId)
              .arg(component.mName);

    qDebug().noquote()
        << QString("  %1 count: %2")
              .arg(signalName)
              .arg(signal.mValues.size());

    qDebug().noquote()
        << QString("  %1 values: %2")
              .arg(signalName)
              .arg(vectorToString(signal.mValues, maxValues));
}
