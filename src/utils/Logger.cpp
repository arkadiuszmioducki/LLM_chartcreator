#include "Logger.h"
#include <QMutexLocker>
#include <QDebug>

namespace utils {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::log(const QString& message) {
    log(Level::Info, message);
}

void Logger::logWarning(const QString& message) {
    log(Level::Warning, message);
}

void Logger::logError(const QString& message) {
    log(Level::Error, message);
}

void Logger::log(Level level, const QString& message) {
    QMutexLocker locker(&m_mutex);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString levelStr = levelToString(level);
    QString formatted = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);

    qDebug().noquote() << formatted;

    if (!m_logFilePath.isEmpty()) {
        QFile file(m_logFilePath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << formatted << "\n";
        }
    }
}

void Logger::setLogFile(const QString& path) {
    QMutexLocker locker(&m_mutex);
    m_logFilePath = path;
}

QString Logger::levelToString(Level level) const {
    switch (level) {
    case Level::Info:    return "INFO";
    case Level::Warning: return "WARN";
    case Level::Error:   return "ERROR";
    case Level::Debug:   return "DEBUG";
    default:             return "INFO";
    }
}

} // namespace utils
