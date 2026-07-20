#include "PlotDisplayMapper.h"

#include <QHash>

namespace
{
    QString lookupDisplayName(const QHash<QString, QString>& mapping,
                              const QString& rawName)
    {
        const auto it =
            mapping.find(rawName);

        if (it == mapping.end())
        {
            return rawName;
        }

        return it.value();
    }
}

QString PlotDisplayMapper::displayStudyTypeName(
    const QString& rawStudyType,
    NameDisplayMode mode)
{
    if (mode == NameDisplayMode::Raw)
    {
        return rawStudyType;
    }

    static const QHash<QString, QString> mapping =
    {
        { "BusbarOutput",     "Busbars" },
        { "MonBranchOutput",  "Branches" },
        { "GeneratorOutput",  "Synchronous machines" },
        { "IndMachOutput",    "Induction machines" },

        { "AVROutput",        "AVR controllers" },
        { "GovernorOutput",   "Governor controllers" }
    };

    return lookupDisplayName(mapping,
                             rawStudyType);
}

QString PlotDisplayMapper::displaySignalName(
    const QString& rawStudyType,
    const QString& rawSignalName,
    NameDisplayMode mode)
{
    if (mode == NameDisplayMode::Raw)
    {
        return rawSignalName;
    }

    if (rawStudyType == "BusbarOutput")
    {
        static const QHash<QString, QString> mapping =
        {
            { "VMag",           "Voltage (PU)" },
            { "VAngle",         "Angle in (deg)" },
            { "UnwrappedAngle", "Unwrapped Angle (deg)" },
            { "Frequency",      "Frequency (PU)" }
        };

        return lookupDisplayName(mapping,
                                 rawSignalName);
    }

    if (rawStudyType == "MonBranchOutput")
    {
        static const QHash<QString, QString> mapping =
        {
            { "SndVoltage",        "Send Voltage (PU)" },
            { "SndAng",            "Send Angle (deg)" },
            { "SndCurrent",        "Send Current (PU)" },
            { "SndRealPow",        "Send Real Power (MW)" },
            { "SndReactPow",       "Send React Power (MVar)" },
            { "SndRealCurr",       "Send Real Current (PU)" },
            { "SndReactCurr",      "Send React Current (PU)" },
            { "SndReactCurrRatio", "Send React Current ratio" },

            { "RcvVoltage",        "Recv Voltage (PU)" },
            { "RecvVoltage",       "Recv Voltage (PU)" },

            { "RecvAng",           "Recv Angle (deg)" },
            { "RcvAng",            "Recv Angle (deg)" },

            { "RcvCurrent",        "Recv Current (PU)" },
            { "RecvCurrent",       "Recv Current (PU)" },

            { "RcvRealPow",        "Recv Real Power (MW)" },
            { "RecvRealPow",       "Recv Real Power (MW)" },

            { "RcvReactPow",       "Recv React Power (MVar)" },
            { "RecvReactPow",      "Recv React Power (MVar)" },

            { "RcvRealCurr",       "Recv Real Current (PU)" },
            { "RecvRealCurr",      "Recv Real Current (PU)" },

            { "RcvReactCurr",      "Recv React Current (PU)" },
            { "RecvReactCurr",     "Recv React Current (PU)" },

            { "RcvReactCurrRatio", "Recv React Current ratio" },
            { "RecvReactCurrRatio","Recv React Current ratio" }
        };

        return lookupDisplayName(mapping,
                                 rawSignalName);
    }

    if (rawStudyType == "GeneratorOutput")
    {
        static const QHash<QString, QString> mapping =
        {
            { "Angle",     "Rotor Angle (deg)" },
            { "FDev",      "Frequency Deviation (PU)" },
            { "Slip",      "Slip (PU)" },
            { "RealPow",   "Real Power Output (MW)" },
            { "ReactPow",  "Reactive Power Output (MVar)" },
            { "PMech",     "Turbine Power (MW)" },
            { "FieldV",    "Field Voltage (PU)" },
            { "FieldCurr", "Field Current (PU)" },
            { "TermV",     "Terminal Voltage (PU)" },
            { "TermCurr",  "Terminal Current (PU)" },
            { "PowFac",    "Power Factor" }
        };

        return lookupDisplayName(mapping,
                                 rawSignalName);
    }

    if (rawStudyType == "IndMachOutput")
    {
        static const QHash<QString, QString> mapping =
        {
            { "Slip",              "Slip (%)" },
            { "InPowMW",           "Real Power Input (MW)" },
            { "InPowMVAR",         "Reactive Power Input (MVar)" },
            { "TermV",             "Terminal Voltage (PU)" },
            { "TermI",             "Terminal Current (PU)" },
            { "LoadTQ",            "Load Torque" },
            { "MotorTQ",           "Motor Torque" },
            { "PowFac",            "Power Factor" },
            { "RotorCurrMag",      "Rotor Current Magnitude (PU)" },
            { "RotorDAxCurr",      "Rotor D-Axis Current (PU)" },
            { "RotorQAxCurr",      "Rotor Q-Axis Current (PU)" },
            { "RotorDAxVInject",   "Rotor D-Axis Volt Inject (PU)" },
            { "RotorQAxVInject",   "Rotor Q-Axis Volt Inject (PU)" }
        };

        return lookupDisplayName(mapping,
                                 rawSignalName);
    }

    return rawSignalName;
}
