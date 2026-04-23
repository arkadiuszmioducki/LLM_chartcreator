#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QStatusBar>
#include <QDockWidget>
#include <memory>

#include "../api/OllamaClient.h"
#include "../core/CodeExecutor.h"
#include "../core/PromptBuilder.h"

/**
 * @file MainWindow.h
 * @brief Glowne okno aplikacji LLM chart creator.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 * @version 1.1.0
 *
 * Zmiany wzgledem v1.0:
 * - Usunieto zakladke diagnostyki sieci (poza zakresem aplikacji)
 * - Dodano panel debugowania (QDockWidget) z logiem promptow i odpowiedzi
 * - Dodano obsluge niestandardowej sciezki do interpretera Python
 * - Naprawiono wybor formatu wyjsciowego (auto-aktualizacja rozszerzenia)
 * - Domyslna sciezka zapisu: Dokumenty/LLM_chart_creator/
 */

namespace gui {

/**
 * @brief Glowne okno aplikacji - generator wykresow Python z LLM.
 *
 * Aplikacja pozwala uzytkownikowi zdefiniowac parametry wykresu,
 * wyslac zapytanie do lokalnego modelu Ollama, obejrzec i wykonac
 * wygenerowany kod Python, a wynik zobaczyc bezposrednio w oknie.
 *
 * Zakladki:
 * - Generator Wykresow  - parametry i dane wejsciowe
 * - Wygenerowany Kod    - podglad i edycja kodu przed wykonaniem
 * - Wyniki Wykonania    - stdout/stderr i podglad obrazu
 *
 * Panel debugowania (QDockWidget) pokazuje pelne JSONy zapytan/odpowiedzi,
 * bledy sieciowe i zdarzenia aplikacji w czasie rzeczywistym.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor glownego okna.
     * @param parent Rodzic Qt
     */
    explicit MainWindow(QWidget* parent = nullptr);

    /**
     * @brief Destruktor.
     */
    ~MainWindow() override = default;

private slots:
    /** @brief Obsluguje klikniecie przycisku Generuj kod. */
    void onGenerateClicked();

    /** @brief Obsluguje klikniecie przycisku Wykonaj kod. */
    void onExecuteClicked();

    /** @brief Obsluguje klikniecie przycisku Anuluj. */
    void onCancelClicked();

    /** @brief Otwiera dialog ustawien. */
    void onSettingsClicked();

    /** @brief Sprawdza dostepnosc serwera Ollama. */
    void onCheckServerClicked();

    /** @brief Czysci kod, output i podglad obrazu. */
    void onClearClicked();

    /** @brief Laduje dane wejsciowe z pliku CSV/TXT. */
    void onLoadDataFileClicked();

    /** @brief Zapisuje wygenerowany kod do pliku .py. */
    void onSaveCodeClicked();

    /** @brief Otwiera wygenerowany obraz w domyslnej przegladarce plikow. */
    void onOpenImageClicked();

    /**
     * @brief Obsluguje nowa linie z stdout skryptu Python.
     * @param line Linia tekstu
     */
    void onOutputLine(const QString& line);

    /** @brief Przelacza widocznosc panelu debugowania. */
    void onToggleDebugPanel();

    /** @brief Czysci zawartosc panelu debugowania. */
    void onClearDebugLog();

private:
    // --- UI budowanie ---

    /** @brief Inicjalizuje caly interfejs graficzny. */
    void setupUi();

    /**
     * @brief Tworzy zakladke generatora wykresow.
     * @return Nowy widget zakladki
     */
    QWidget* createChartTab();

    /**
     * @brief Tworzy zakladke podgladu wygenerowanego kodu.
     * @return Nowy widget zakladki
     */
    QWidget* createCodeTab();

    /**
     * @brief Tworzy zakladke wynikow wykonania i podgladu obrazu.
     * @return Nowy widget zakladki
     */
    QWidget* createOutputTab();

    /**
     * @brief Tworzy panel debugowania jako QDockWidget.
     * @return Skonfigurowany dock widget
     */
    QDockWidget* createDebugDock();

    // --- Logika ---

    /**
     * @brief Zbiera parametry wykresu z kontrolek GUI.
     *
     * Wymusza rozszerzenie pliku zgodne z wybranym formatem w combo.
     * @return Wypelniona struktura ChartParameters
     */
    core::ChartParameters collectChartParams() const;

    /**
     * @brief Waliduje dane wejsciowe przed wyslaniem zapytania.
     * @return true jesli dane sa kompletne i poprawne
     */
    bool validateInputs() const;

    /**
     * @brief Ustawia UI w tryb "operacja w toku" (blokuje przyciski).
     * @param busy true = trwa operacja
     */
    void setBusy(bool busy);

    /**
     * @brief Wyswietla komunikat w pasku stanu.
     * @param message Tresc komunikatu
     * @param isError Czy to blad (czerwony kolor tekstu)
     */
    void setStatus(const QString& message, bool isError = false);

    /**
     * @brief Wyswietla wygenerowany kod w zakladce podgladu.
     * @param code Kod Python
     */
    void displayCode(const QString& code);

    /**
     * @brief Wyswietla wyniki wykonania skryptu w zakladce Wyniki.
     * @param result Wynik wykonania z CodeExecutor
     */
    void displayExecutionResult(const core::ExecutionResult& result);

    /**
     * @brief Wyswietla wygenerowany wykres w zakladce Wyniki.
     * @param imagePath Sciezka do pliku obrazu (PNG/PDF/SVG/JPG)
     */
    void displayImage(const QString& imagePath);

    /**
     * @brief Dodaje wpis do panelu debugowania.
     *
     * Wpis jest opatrzony znacznikiem czasu i kategoria.
     * @param category Kategoria, np. "REQUEST", "RESPONSE", "ERROR"
     * @param content Tresc wpisu
     */
    void debugLog(const QString& category, const QString& content);

    /**
     * @brief Wczytuje ustawienia z QSettings (rejestr/plik ini).
     *
     * Wczytuje: URL Ollamy, nazwe modelu, sciezke do Pythona.
     */
    void loadSettings();

    /**
     * @brief Zapisuje biezace ustawienia do QSettings.
     */
    void saveSettings();

    // --- Backend ---
    std::unique_ptr<api::OllamaClient>   m_ollamaClient;   ///< Klient REST API Ollama
    std::unique_ptr<core::CodeExecutor>  m_codeExecutor;   ///< Executor kodu Python
    std::unique_ptr<core::PromptBuilder> m_promptBuilder;  ///< Budowniczy promptow

    // --- Stan ---
    QString m_currentCode;      ///< Aktualnie wygenerowany/edytowany kod Python
    QString m_ollamaUrl;        ///< URL serwera Ollama
    QString m_modelName;        ///< Nazwa modelu LLM
    QString m_pythonPath;       ///< Sciezka do interpretera Python (pusta = auto)
    QString m_lastImagePath;    ///< Sciezka ostatnio wygenerowanego wykresu

    // --- Glowny layout ---
    QTabWidget*    m_tabWidget         = nullptr;

    // === Zakladka: Generator Wykresow ===
    QComboBox*     m_chartTypeCombo    = nullptr;  ///< Typ wykresu (Line/Bar/...)
    QLineEdit*     m_chartTitleEdit    = nullptr;  ///< Tytul wykresu
    QLineEdit*     m_xLabelEdit        = nullptr;  ///< Etykieta osi X
    QLineEdit*     m_yLabelEdit        = nullptr;  ///< Etykieta osi Y
    QTextEdit*     m_dataEdit          = nullptr;  ///< Dane wejsciowe (CSV/JSON/opis)
    QComboBox*     m_colorSchemeCombo  = nullptr;  ///< Schemat kolorow
    QCheckBox*     m_showGridCheck     = nullptr;  ///< Pokazuj siatke
    QCheckBox*     m_showLegendCheck   = nullptr;  ///< Pokazuj legende
    QComboBox*     m_outputFormatCombo = nullptr;  ///< Format wyjsciowy (png/pdf/svg/jpg)
    QLineEdit*     m_outputPathEdit    = nullptr;  ///< Pelna sciezka zapisu
    QSpinBox*      m_figWidthSpin      = nullptr;  ///< Szerokosc wykresu w calach
    QSpinBox*      m_figHeightSpin     = nullptr;  ///< Wysokosc wykresu w calach
    QTextEdit*     m_additionalInfoEdit= nullptr;  ///< Dodatkowe instrukcje dla modelu
    QPushButton*   m_loadDataBtn       = nullptr;  ///< Przycisk ladowania pliku danych

    // === Zakladka: Wygenerowany Kod ===
    QPlainTextEdit* m_codeEdit         = nullptr;  ///< Edytor kodu Python
    QPushButton*    m_saveCodeBtn      = nullptr;  ///< Zapisz kod do pliku

    // === Zakladka: Wyniki Wykonania ===
    QPlainTextEdit* m_outputEdit       = nullptr;  ///< Stdout/stderr skryptu
    QLabel*         m_imageLabel       = nullptr;  ///< Podglad wygenerowanego wykresu
    QPushButton*    m_openImageBtn     = nullptr;  ///< Otworz obraz zewnetrznie

    // === Panel debugowania ===
    QDockWidget*    m_debugDock        = nullptr;  ///< Dokowalny panel debugu
    QPlainTextEdit* m_debugEdit        = nullptr;  ///< Log debugowania
    QPushButton*    m_toggleDebugBtn   = nullptr;  ///< Przycisk pokazania/ukrycia panelu

    // === Kontrolki glowne ===
    QPushButton*   m_generateBtn       = nullptr;  ///< Generuj kod przez LLM
    QPushButton*   m_executeBtn        = nullptr;  ///< Wykonaj wygenerowany kod
    QPushButton*   m_cancelBtn         = nullptr;  ///< Anuluj trwajaca operacje
    QPushButton*   m_clearBtn          = nullptr;  ///< Wyczysc wszystkie pola
    QProgressBar*  m_progressBar       = nullptr;  ///< Pasek postepu (nieokreslony)
    QLabel*        m_serverStatusLabel = nullptr;  ///< Wskaznik stanu serwera Ollama
};

} // namespace gui
