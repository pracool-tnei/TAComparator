#pragma once

#include <QFile>
#include <QString>
#include <QTextStream>

class TextFileReader
{
public:
    TextFileReader() = default;
    ~TextFileReader();

    bool open(const QString& fileName);

    bool readNextLine(QString& line);

    bool isOpen() const;

    int currentLineNumber() const;

    void close();

private:
    QFile mFile;
    QTextStream mStream;

    int mCurrentLineNumber = 0;
};