#pragma once

#include <QObject>
#include <QString>
#include <QProcess>
#include <functional>

/**
 * @file CodeExecutor.h
 * @brief Klasa odpowiedzialna za ekstrakcję i wykonanie kodu Python.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 */

namespace core {

/**
 * @brief Wynik wykonania kodu Python.
 */
struct ExecutionResult {
    bool success;        ///< Czy wykonanie zakończyło się bez błędu
    QString stdout_output; ///< Standardowe wyjście
    QString stderr_output; ///< Wyjście błędów
    int exitCode;        ///< Kod wyjścia procesu
    QString generatedImagePath; ///< Ścieżka do wygenerowanego pliku (jeśli dotyczy)
};

/**
 * @brief Klasa do ekstrakcji kodu z odpowiedzi LLM i jego wykonania.
 *
 * Obsługuje wielowątkowe wykonanie kodu Python za pomocą QProcess,
 * wyodrębnia kod z odpowiedzi modelu (usuwając znaczniki markdown),
 * oraz automatycznie wykrywa wygenerowane pliki graficzne.
 *
 * @example
 * @code
 * CodeExecutor executor;
 * QString llmResponse = "```python\nprint('Hello')\n```";
 * QString code = executor.extractCode(llmResponse);
 * executor.executeAsync(code, [](ExecutionResult r) {
 *     qDebug() << r.stdout_output;
 * });
 * @endcode
 */
class CodeExecutor : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor.
     * @param parent Rodzic Qt
     */
    explicit CodeExecutor(QObject* parent = nullptr);

    /**
     * @brief Destruktor - kończy ewentualny działający proces.
     */
    ~CodeExecutor() override;

    /**
     * @brief Wyodrębnia kod Python z odpowiedzi modelu językowego.
     *
     * Obsługuje kod opakowany w znaczniki ```python...``` lub ```...```,
     * a także "czysty" kod bez znaczników.
     *
     * @param llmResponse Pełna odpowiedź z modelu
     * @return Wyodrębniony kod Python
     */
    QString extractCode(const QString& llmResponse) const;

    /**
     * @brief Asynchronicznie wykonuje kod Python.
     * @param code Kod Python do wykonania
     * @param callback Funkcja wywoływana po zakończeniu
     * @param timeoutMs Maksymalny czas wykonania w ms (domyślnie 30000)
     */
    void executeAsync(const QString& code,
                      std::function<void(ExecutionResult)> callback,
                      int timeoutMs = 30000);

    /**
     * @brief Sprawdza czy Python jest dostępny w systemie.
     * @return Ścieżka do Python lub pusty string jeśli niedostępny
     */
    static QString findPythonExecutable();

    /**
     * @brief Anuluje trwające wykonanie kodu.
     */
    void cancelExecution();

    /**
     * @brief Ustawia sciezke do interpretera Python.
     *
     * Pozwala uzytkownikowi wskazac niestandardowy interpreter
     * (np. virtualenv, conda, Python z Microsoft Store).
     * Jesli pusty string lub nullptr - uzywa auto-detekcji.
     * @param path Pelna sciezka do pliku wykonywalnego Python
     */
    void setPythonExecutable(const QString& path);

    /**
     * @brief Sprawdza czy trwa wykonanie kodu.
     * @return true jeśli kod jest aktualnie wykonywany
     */
    bool isRunning() const;

    /**
     * @brief Instaluje wymagane biblioteki Python.
     * @param packages Lista pakietów do zainstalowania
     * @param callback Wywoływane po zakończeniu instalacji
     */
    void installDependencies(const QStringList& packages,
                             std::function<void(bool, QString)> callback);

signals:
    /**
     * @brief Sygnał emitowany gdy jest nowy output ze stdout.
     * @param text Nowa linia tekstu
     */
    void outputLine(QString text);

    /**
     * @brief Sygnał emitowany gdy wykonanie się zakończyło.
     * @param result Wynik wykonania
     */
    void executionFinished(ExecutionResult result);

private:
    /**
     * @brief Zapisuje kod do tymczasowego pliku .py.
     * @param code Kod do zapisania
     * @return Ścieżka do pliku lub pusty string w przypadku błędu
     */
    QString saveToTempFile(const QString& code) const;

    /**
     * @brief Wyodrębnia ścieżkę do wygenerowanego pliku z output.
     * @param output Standardowe wyjście skryptu
     * @return Ścieżka do pliku lub pusty string
     */
    QString extractGeneratedFilePath(const QString& output) const;

    QProcess* m_process;           ///< Proces Python
    bool m_isRunning;              ///< Flaga wykonywania
    QString m_pythonExecutable;    ///< Ścieżka do interpretera Python
};

} // namespace core
