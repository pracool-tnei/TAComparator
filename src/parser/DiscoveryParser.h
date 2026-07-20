#pragma once

#include <QString>
#include <QStringList>

class Study;
class TextFileReader;

class DiscoveryParser
{
public:
    bool parse(const QString& fileName, Study& study);

private:
    void parseDefineBlock(TextFileReader& reader,
                          const QString& firstLine);

    void parseSystemMap(TextFileReader& reader);

    void parseRecord(TextFileReader& reader,
                     const QString& objectType);

    void parseFieldLine(const QString& line);

    void parseSystemMapRecord(const QString& objectType,
                              const QString& line);
	
	void parseHeaderMetadataLine(const QString& line);

    bool isHeaderLine(const QString& line) const;
    bool isDefineLine(const QString& line) const;
    bool isRecordLine(const QString& line) const;

private:
    Study* mStudy = nullptr;

    QString mCurrentObject;
    QStringList mCurrentFields;
};