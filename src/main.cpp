/**
 * @file main.cpp
 * @brief Punkt wejściowy aplikacji LLM chart creator.
 *
 * Uruchamia aplikację Qt i wyświetla główne okno.
 * Aplikacja wymaga lokalnie uruchomionego serwera Ollama.
 *
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 * @version 1.0.0
 */

#include <QApplication>
#include <QIcon>
#include <QDir>

#include "gui/MainWindow.h"
#include "utils/Logger.h"

/**
 * @brief Główna funkcja aplikacji.
 * @param argc Liczba argumentów
 * @param argv Tablica argumentów
 * @return Kod wyjścia aplikacji
 */
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("LLM chart creator");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("");

    // Setup logging
    QString logPath = QDir::tempPath() + "/LLM_chart_creator.log";
    utils::Logger::instance().setLogFile(logPath);
    utils::Logger::instance().log("=== LLM chart creator v1.0.0 ===");
    utils::Logger::instance().log("Log: " + logPath);

    // Apply modern style
    app.setStyle("Fusion");

    gui::MainWindow window;
    window.show();

    return app.exec();
}
