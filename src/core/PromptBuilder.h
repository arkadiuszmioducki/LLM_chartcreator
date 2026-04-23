#pragma once

#include <QString>
#include <QStringList>

/**
 * @file PromptBuilder.h
 * @brief Klasa do budowania promptów dla generatora kodu Python.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 */

namespace core {

/**
 * @brief Typ wykresu do wygenerowania.
 */
enum class ChartType {
    Line,       ///< Wykres liniowy
    Bar,        ///< Wykres słupkowy
    Scatter,    ///< Wykres punktowy
    Histogram,  ///< Histogram
    Pie         ///< Wykres kołowy
};

/**
 * @brief Parametry wykresu podawane przez użytkownika.
 */
struct ChartParameters {
    ChartType type = ChartType::Line; ///< Typ wykresu
    QString title;            ///< Tytuł wykresu
    QString xLabel;           ///< Etykieta osi X
    QString yLabel;           ///< Etykieta osi Y
    QString dataInput;        ///< Dane wejściowe (CSV lub JSON)
    QString colorScheme;      ///< Schemat kolorów
    bool showGrid = true;     ///< Czy pokazać siatkę
    bool showLegend = true;   ///< Czy pokazać legendę
    QString outputFormat;     ///< Format wyjściowy (png, pdf, svg)
    QString outputPath;       ///< Ścieżka zapisu pliku
    int figWidth = 10;        ///< Szerokość wykresu (cale)
    int figHeight = 6;        ///< Wysokość wykresu (cale)
    QString additionalInfo;   ///< Dodatkowe instrukcje dla modelu
};

/**
 * @brief Typ zadania - do czego służy generator.
 */
enum class TaskType {
    ChartGeneration,    ///< Generacja wykresów
    NetworkDiagnosis,   ///< Diagnostyka sieci (ping)
    SystemCommand,      ///< Ogólna komenda systemowa
    DataAnalysis        ///< Analiza danych
};

/**
 * @brief Klasa budująca system prompt i user prompt dla Ollama.
 *
 * Odpowiada za tworzenie dobrze zoptymalizowanych promptów
 * zwiększających jakość generowanego kodu Python.
 *
 * @example
 * @code
 * ChartParameters params;
 * params.type = ChartType::Bar;
 * params.title = "Sprzedaż miesięczna";
 * params.dataInput = "1,2,3,4,5\n100,150,120,180,90";
 *
 * PromptBuilder builder;
 * auto system = builder.buildSystemPrompt(TaskType::ChartGeneration);
 * auto user   = builder.buildUserPrompt(params);
 * @endcode
 */
class PromptBuilder {
public:
    /**
     * @brief Konstruktor domyślny.
     */
    PromptBuilder() = default;

    /**
     * @brief Buduje system prompt dla danego typu zadania.
     * @param task Typ zadania
     * @return System prompt jako QString
     */
    QString buildSystemPrompt(TaskType task) const;

    /**
     * @brief Buduje user prompt na podstawie parametrów wykresu.
     * @param params Parametry wykresu
     * @return User prompt jako QString
     */
    QString buildUserPrompt(const ChartParameters& params) const;

    /**
     * @brief Buduje user prompt dla diagnostyki sieci.
     * @param hosts Lista hostów do sprawdzenia
     * @param pingCount Liczba pakietów ping
     * @param timeout Timeout w sekundach
     * @return User prompt jako QString
     */
    QString buildNetworkPrompt(const QStringList& hosts, int pingCount, int timeout) const;

    /**
     * @brief Buduje user prompt dla ogólnego polecenia systemowego.
     * @param description Opis polecenia
     * @param osType System operacyjny ("windows" lub "linux")
     * @return User prompt jako QString
     */
    QString buildSystemCommandPrompt(const QString& description, const QString& osType) const;

    /**
     * @brief Konwertuje typ wykresu na string.
     * @param type Typ wykresu
     * @return Nazwa typu jako QString
     */
    static QString chartTypeToString(ChartType type);

    /**
     * @brief Konwertuje string na typ wykresu.
     * @param str Nazwa typu
     * @return Odpowiadający ChartType
     */
    static ChartType stringToChartType(const QString& str);

private:
    /**
     * @brief Formatuje dane CSV/JSON do includowania w prompcie.
     * @param data Surowe dane
     * @return Sformatowany string
     */
    QString formatDataForPrompt(const QString& data) const;

    /**
     * @brief Mapuje nazwę schematu kolorów na bezpieczną listę kolorów hex.
     *
     * Unika przestarzałego plt.cm.get_cmap() i błędów z tablicami numpy jako kolorami.
     * @param scheme Nazwa schematu (np. "tab10", "viridis")
     * @return Python lista kolorów jako string, np. "['#1f77b4','#ff7f0e',...]"
     */
    QString mapColorScheme(const QString& scheme) const;
};

} // namespace core
