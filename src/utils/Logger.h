#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

/**
 * @file Logger.h
 * @brief Singleton do logowania zdarzeń aplikacji.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 */

namespace utils {

/**
 * @brief Singleton do logowania zdarzeń do pliku i konsoli.
 *
 * Używa QMutex do bezpiecznego wielowątkowego zapisu.
 *
 * @example
 * @code
 * Logger::instance().log("Aplikacja uruchomiona");
 * Logger::instance().logError("Błąd połączenia");
 * @endcode
 */
class Logger {
public:
    /**
     * @brief Poziom logowania.
     */
    enum class Level { Info, Warning, Error, Debug };

    /**
     * @brief Zwraca instancję singletona.
     * @return Referencja do instancji Logger
     */
    static Logger& instance();

    /**
     * @brief Loguje wiadomość informacyjną.
     * @param message Treść wiadomości
     */
    void log(const QString& message);

    /**
     * @brief Loguje ostrzeżenie.
     * @param message Treść ostrzeżenia
     */
    void logWarning(const QString& message);

    /**
     * @brief Loguje błąd.
     * @param message Treść błędu
     */
    void logError(const QString& message);

    /**
     * @brief Loguje wiadomość z określonym poziomem.
     * @param level Poziom logowania
     * @param message Treść wiadomości
     */
    void log(Level level, const QString& message);

    /**
     * @brief Ustawia ścieżkę pliku logu.
     * @param path Ścieżka do pliku
     */
    void setLogFile(const QString& path);

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    QString levelToString(Level level) const;

    QMutex m_mutex;        ///< Mutex do wielowątkowego dostępu
    QString m_logFilePath; ///< Ścieżka do pliku logu
};

} // namespace utils
