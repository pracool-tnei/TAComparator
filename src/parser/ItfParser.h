#pragma once

#include <QString>

class Study;

class ItfParser
{
public:
    bool parse(const QString& fileName, Study& study);
};