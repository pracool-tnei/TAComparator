#pragma once

#include <QString>

enum class NameDisplayMode
{
    Raw,
    IpsaFriendly
};

namespace PlotDisplayMapper
{
    QString displayStudyTypeName(const QString& rawStudyType,
                                 NameDisplayMode mode);

    QString displaySignalName(const QString& rawStudyType,
                              const QString& rawSignalName,
                              NameDisplayMode mode);
}
