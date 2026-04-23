#pragma once

#include <QString>
#include <QStringList>

/**
 * @file FileUtils.h
 * @brief Narzędzia do operacji na plikach.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 */

namespace utils {

/**
 * @brief Zbiór statycznych narzędzi do operacji na plikach.
 */
class FileUtils {
public:
    /**
     * @brief Odczytuje zawartość pliku tekstowego.
     * @param path Ścieżka do pliku
     * @param ok Wskaźnik sukcesu (opcjonalny)
     * @return Zawartość pliku lub pusty string przy błędzie
     */
    static QString readTextFile(const QString& path, bool* ok = nullptr);

    /**
     * @brief Zapisuje tekst do pliku.
     * @param path Ścieżka do pliku
     * @param content Zawartość do zapisania
     * @return true jeśli zapis się powiódł
     */
    static bool writeTextFile(const QString& path, const QString& content);

    /**
     * @brief Sprawdza czy plik istnieje.
     * @param path Ścieżka do pliku
     * @return true jeśli plik istnieje
     */
    static bool fileExists(const QString& path);

    /**
     * @brief Zwraca rozszerzenie pliku.
     * @param path Ścieżka do pliku
     * @return Rozszerzenie (bez kropki) lub pusty string
     */
    static QString fileExtension(const QString& path);

    /**
     * @brief Parsuje dane CSV i zwraca listę wierszy.
     * @param csvContent Zawartość CSV
     * @param delimiter Separator (domyślnie ',')
     * @return Lista wierszy, każdy jako lista wartości
     */
    static QList<QStringList> parseCsv(const QString& csvContent, QChar delimiter = ',');

    /**
     * @brief Generuje unikalną ścieżkę pliku w katalogu tymczasowym.
     * @param prefix Prefiks nazwy pliku
     * @param extension Rozszerzenie (z kropką, np. ".png")
     * @return Unikalna ścieżka pliku
     */
    static QString uniqueTempPath(const QString& prefix, const QString& extension);
};

} // namespace utils
