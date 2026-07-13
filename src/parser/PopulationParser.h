#pragma once

#include <QString>

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
                     const QString& objectType);
    void parseDataLine(const QString& recordName,
                                         const QString& line);


    Study* mStudy = nullptr;
};