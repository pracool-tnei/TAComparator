#pragma once

#include <QString>
#include <QStringList>

class Study;
class TextFileReader;
class ObjectGroup;

class PopulationParser
{
public:
    bool parse(const QString& fileName, Study& study);

private:
    void parseSystemMap(TextFileReader& reader);
    void parseStep(TextFileReader& reader);

    void parseRecord(TextFileReader& reader,
                     const QString& recordName);

    void parseDataLine(const QString& recordName,
                       const QStringList& tokens);

    void populateBusbarOutput(const QStringList& tokens);

    void populateMonBranchOutput(const QStringList& tokens);

    void populateGeneratorOutput(const QStringList& tokens);

	void populateAVROutput(const QStringList& tokens);

	void populateGovernorOutput(const QStringList& tokens);

	void populateRunStep(const QStringList& tokens);

	//helper function for debugging prints
	void printPopulationSummary() const;
	void printPopulationSummaryRunStep() const;
	void printPopulationSummaryBusbar() const;
	void printPopulationSummaryMonBranch() const;
	void printPopulationSummaryGenerator() const;
	void printPopulationSummaryAVR() const;
	void printPopulationSummaryGovernor() const;	

	void printSignalVector(const QString& groupName,
                       int componentId,
                       const QString& signalName,
                       int maxValues = 40) const;

private:
    Study* mStudy = nullptr;

    int mBusbarOutputRowsStored = 0;

    int mMonBranchOutputRowsStored = 0;
    int mMonBranchOutputRowsSkipped = 0;

    int mGeneratorOutputRowsStored = 0;

	int mAVROutputRowsStored = 0;

	int mGovernorOutputRowsStored = 0;
	int mGovernorOutputRowsSkipped = 0;

	int mRunStepRowsStored = 0;
};
