#include "TextFileReader.h"

TextFileReader::~TextFileReader()
{
    close();
}

bool TextFileReader::open(const QString& fileName)
{
    mFile.setFileName(fileName);

    if (!mFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    mStream.setDevice(&mFile);

    mCurrentLineNumber = 0;

    return true;
}

bool TextFileReader::readNextLine(QString& line)
{
    if (!mFile.isOpen())
    {
        return false;
    }

    if (mStream.atEnd())
    {
        return false;
    }

    line = mStream.readLine();

    ++mCurrentLineNumber;

    return true;
}

bool TextFileReader::isOpen() const
{
    return mFile.isOpen();
}

int TextFileReader::currentLineNumber() const
{
    return mCurrentLineNumber;
}

void TextFileReader::close()
{
    if (mFile.isOpen())
    {
        mFile.close();
    }
}