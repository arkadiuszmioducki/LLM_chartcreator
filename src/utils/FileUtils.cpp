#include "FileUtils.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>

namespace utils {

QString FileUtils::readTextFile(const QString& path, bool* ok) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok) *ok = false;
        return {};
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll();
    if (ok) *ok = true;
    return content;
}

bool FileUtils::writeTextFile(const QString& path, const QString& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << content;
    return true;
}

bool FileUtils::fileExists(const QString& path) {
    return QFileInfo::exists(path);
}

QString FileUtils::fileExtension(const QString& path) {
    return QFileInfo(path).suffix().toLower();
}

QList<QStringList> FileUtils::parseCsv(const QString& csvContent, QChar delimiter) {
    QList<QStringList> result;
    QStringList lines = csvContent.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        result.append(line.split(delimiter));
    }
    return result;
}

QString FileUtils::uniqueTempPath(const QString& prefix, const QString& extension) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString pid = QString::number(QCoreApplication::applicationPid());
    return QDir::tempPath() + "/" + prefix + "_" + timestamp + "_" + pid + extension;
}

} // namespace utils
