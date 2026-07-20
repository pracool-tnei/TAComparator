#pragma once

#include <QFile>
#include <QMutex>
#include <QString>
#include <QtGlobal>

class Logger
{
public:
    enum class Level
    {
        Debug = 0,
        Info,
        Warning,
        Error,
        Off
    };

    static Logger& instance();

    void install();

    void setMinimumLevel(Level level);
    Level minimumLevel() const;

    void setLogToConsole(bool enabled);
    void setLogToFile(bool enabled);
    void setLogFilePath(const QString& filePath);

private:
    Logger() = default;
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void messageHandler(QtMsgType type,
                               const QMessageLogContext& context,
                               const QString& message);

    void write(Level level,
               const QMessageLogContext& context,
               const QString& message);

    bool shouldLog(Level level) const;

    Level qtTypeToLevel(QtMsgType type) const;
    QString levelToString(Level level) const;
    QString locationToString(const QMessageLogContext& context) const;
    QString functionToString(const QMessageLogContext& context) const;

private:
    Level mMinimumLevel = Level::Debug;

    bool mLogToConsole = true;
    bool mLogToFile = false;

    QString mLogFilePath = "TAComparator.log";
    QFile mLogFile;

    mutable QMutex mMutex;
};
