#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

/**
 * @file SettingsDialog.h
 * @brief Dialog ustawien aplikacji LLM chart creator.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 *
 * Pozwala uzytkownikowi skonfigurowac:
 * - URL serwera Ollama
 * - Nazwe modelu LLM
 * - Sciezke do niestandardowego interpretera Python (opcjonalnie)
 */

namespace gui {

/**
 * @brief Modalny dialog konfiguracji polaczenia i srodowiska.
 *
 * Zawiera:
 * - Pole URL serwera Ollama (domyslnie http://localhost:11434)
 * - Pole nazwy modelu (domyslnie gemma4:e2b)
 * - Pole sciezki do interpretera Python (puste = auto-detekcja)
 * - Przycisk testu polaczenia z Ollama
 * - Przycisk wyboru pliku Python (QFileDialog)
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor dialogu ustawien.
     * @param currentUrl    Aktualny URL serwera Ollama
     * @param currentModel  Aktualna nazwa modelu
     * @param currentPython Aktualna sciezka do Pythona (pusta = auto)
     * @param parent        Rodzic Qt
     */
    explicit SettingsDialog(const QString& currentUrl,
                            const QString& currentModel,
                            const QString& currentPython,
                            QWidget* parent = nullptr);

    /**
     * @brief Zwraca URL serwera Ollama wprowadzony przez uzytkownika.
     * @return URL jako QString
     */
    QString ollamaUrl() const;

    /**
     * @brief Zwraca nazwe modelu wprowadzona przez uzytkownika.
     * @return Nazwa modelu
     */
    QString modelName() const;

    /**
     * @brief Zwraca sciezke do interpretera Python.
     *
     * Jesli uzytkownik nie podal sciezki, zwraca pusty string
     * (oznacza: uzyj auto-detekcji).
     * @return Pelna sciezka lub pusty string
     */
    QString pythonPath() const;

private slots:
    /** @brief Sprawdza dostepnosc serwera Ollama i wyswietla wynik. */
    void onTestConnection();

    /** @brief Otwiera QFileDialog do wyboru pliku wykonywalnego Python. */
    void onBrowsePython();

private:
    QLineEdit* m_urlEdit     = nullptr;  ///< Pole URL Ollamy
    QLineEdit* m_modelEdit   = nullptr;  ///< Pole nazwy modelu
    QLineEdit* m_pythonEdit  = nullptr;  ///< Pole sciezki do Pythona
    QLabel*    m_statusLabel = nullptr;  ///< Etykieta wyniku testu polaczenia
};

} // namespace gui
