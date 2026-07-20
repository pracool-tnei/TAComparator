#include "Logger.h"

#include <QDateTime>
#include <QFileInfo>
#include <QIODevice>
#include <QMutexLocker>
#include <QTextStream>

#include <cstdio>
#include <cstdlib>

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

Logger::~Logger()
{
    if (mLogFile.isOpen())
    {
        mLogFile.close();
    }
}

void Logger::install()
{
    qInstallMessageHandler(Logger::messageHandler);
}

void Logger::setMinimumLevel(Level level)
{
    QMutexLocker locker(&mMutex);
    mMinimumLevel = level;
}

Logger::Level Logger::minimumLevel() const
{
    QMutexLocker locker(&mMutex);
    return mMinimumLevel;
}

void Logger::setLogToConsole(bool enabled)
{
    QMutexLocker locker(&mMutex);
    mLogToConsole = enabled;
}

void Logger::setLogToFile(bool enabled)
{
    QMutexLocker locker(&mMutex);

    mLogToFile = enabled;

    if (!mLogToFile && mLogFile.isOpen())
    {
        mLogFile.close();
    }
}

void Logger::setLogFilePath(const QString& filePath)
{
    QMutexLocker locker(&mMutex);

    mLogFilePath = filePath;

    if (mLogFile.isOpen())
    {
        mLogFile.close();
    }

    mLogFile.setFileName(mLogFilePath);
}

void Logger::messageHandler(QtMsgType type,
                            const QMessageLogContext& context,
                            const QString& message)
{
    Logger& logger = Logger::instance();

    const Level level =
        logger.qtTypeToLevel(type);

    if (!logger.shouldLog(level))
    {
        return;
    }

    logger.write(level,
                 context,
                 message);

    if (type == QtFatalMsg)
    {
        std::abort();
    }
}

bool Logger::shouldLog(Level level) const
{
    QMutexLocker locker(&mMutex);

    if (mMinimumLevel == Level::Off)
    {
        return false;
    }

    return static_cast<int>(level) >=
           static_cast<int>(mMinimumLevel);
}

void Logger::write(Level level,
                   const QMessageLogContext& context,
                   const QString& message)
{
    QMutexLocker locker(&mMutex);

    const QString timestamp =
        QDateTime::currentDateTime()
            .toString("yyyy-MM-dd hh:mm:ss.zzz");

    const QString formattedMessage =
        QString("[%1] [%2] [%3] [%4] %5")
            .arg(timestamp)
            .arg(levelToString(level))
            .arg(locationToString(context))
            .arg(functionToString(context))
            .arg(message);

    if (mLogToConsole)
    {
        std::fprintf(stderr,
                     "%s\n",
                     formattedMessage.toLocal8Bit().constData());

        std::fflush(stderr);
    }

    if (mLogToFile)
    {
        if (!mLogFile.isOpen())
        {
            mLogFile.setFileName(mLogFilePath);

            mLogFile.open(QIODevice::Append |
                          QIODevice::Text);
        }

        if (mLogFile.isOpen())
        {
            QTextStream stream(&mLogFile);
            stream << formattedMessage << Qt::endl;
        }
    }
}

Logger::Level Logger::qtTypeToLevel(QtMsgType type) const
{
    switch (type)
    {
    case QtDebugMsg:
        return Level::Debug;

    case QtInfoMsg:
        return Level::Info;

    case QtWarningMsg:
        return Level::Warning;

    case QtCriticalMsg:
        return Level::Error;

    case QtFatalMsg:
        return Level::Error;
    }

    return Level::Debug;
}

QString Logger::levelToString(Level level) const
{
    switch (level)
    {
    case Level::Debug:
        return "DEBUG";

    case Level::Info:
        return "INFO";

    case Level::Warning:
        return "WARNING";

    case Level::Error:
        return "ERROR";

    case Level::Off:
        return "OFF";
    }

    return "UNKNOWN";
}

QString Logger::locationToString(
    const QMessageLogContext& context) const
{
    if (!context.file)
    {
        return "unknown:0";
    }

    const QFileInfo fileInfo(context.file);

    return QString("%1:%2")
        .arg(fileInfo.fileName())
        .arg(context.line);
}

QString Logger::functionToString(
    const QMessageLogContext& context) const
{
    if (!context.function)
    {
        return "unknown";
    }

    QString functionName =
        QString::fromUtf8(context.function);

    /*
     * Optional cleanup:
     * C++ function names can be long, for example:
     * StudyBrowserWidget::updateSignalSummary()
     *
     * This keeps them readable but still useful.
     */
    return functionName;
}
