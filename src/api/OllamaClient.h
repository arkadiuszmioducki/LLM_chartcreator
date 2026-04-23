#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QString>
#include <functional>

/**
 * @file OllamaClient.h
 * @brief Klient REST API do komunikacji z lokalnym serwerem Ollama.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 */

namespace api {

/**
 * @brief Struktura przechowująca odpowiedź z Ollama API.
 */
struct OllamaResponse {
    bool success;       ///< Czy zapytanie zakończyło się sukcesem
    QString response;   ///< Treść odpowiedzi modelu
    QString errorMsg;   ///< Komunikat błędu (jeśli success == false)
    int statusCode;     ///< Kod HTTP odpowiedzi
};

/**
 * @brief Parametry zapytania do modelu językowego.
 */
struct OllamaRequest {
    QString model;      ///< Nazwa modelu (np. "gemma2:2b")
    QString systemPrompt; ///< System prompt definiujący rolę modelu
    QString userPrompt;   ///< Zapytanie użytkownika
    bool stream = false;  ///< Czy używać streamingu (false = jednorazowa odpowiedź)
    double temperature = 0.2; ///< Temperatura generacji (0.0 - 1.0)
};

/**
 * @brief Klient do komunikacji z Ollama REST API.
 *
 * Klasa umożliwia asynchroniczne i synchroniczne wysyłanie zapytań
 * do lokalnie uruchomionego serwera Ollama. Obsługuje wyjątki sieciowe
 * i brak dostępności usługi.
 *
 * @example
 * @code
 * OllamaClient client("http://localhost:11434");
 * OllamaRequest req;
 * req.model = "gemma2:2b";
 * req.systemPrompt = "You are a Python code generator.";
 * req.userPrompt = "Write a function to sort a list.";
 * client.sendRequest(req, [](OllamaResponse resp) {
 *     if (resp.success) qDebug() << resp.response;
 * });
 * @endcode
 */
class OllamaClient : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klienta Ollama.
     * @param baseUrl Bazowy URL serwera Ollama (domyślnie http://localhost:11434)
     * @param parent Rodzic Qt
     */
    explicit OllamaClient(const QString& baseUrl = "http://localhost:11434",
                          QObject* parent = nullptr);

    /**
     * @brief Destruktor.
     */
    ~OllamaClient() override = default;

    /**
     * @brief Wysyła asynchroniczne zapytanie do modelu językowego.
     * @param request Parametry zapytania
     * @param callback Funkcja wywoływana po otrzymaniu odpowiedzi
     */
    void sendRequest(const OllamaRequest& request,
                     std::function<void(OllamaResponse)> callback);

    /**
     * @brief Sprawdza dostępność serwera Ollama.
     * @return true jeśli serwer jest dostępny
     */
    bool isServerAvailable() const;

    /**
     * @brief Pobiera listę dostępnych modeli.
     * @param callback Funkcja wywoływana z listą modeli
     */
    void fetchAvailableModels(std::function<void(QStringList, QString)> callback);

    /**
     * @brief Ustawia bazowy URL serwera.
     * @param url Nowy URL
     */
    void setBaseUrl(const QString& url);

    /**
     * @brief Zwraca bazowy URL serwera.
     * @return URL serwera
     */
    QString baseUrl() const;

    /**
     * @brief Ustawia timeout zapytania w milisekundach.
     * @param ms Timeout w ms (domyślnie 60000)
     */
    void setTimeout(int ms);

signals:
    /**
     * @brief Sygnał emitowany po otrzymaniu odpowiedzi.
     * @param response Odpowiedź z API
     */
    void responseReceived(OllamaResponse response);

    /**
     * @brief Sygnał emitowany w przypadku błędu sieciowego.
     * @param errorMessage Opis błędu
     */
    void networkError(QString errorMessage);

private:
    /**
     * @brief Buduje JSON body dla zapytania generate.
     * @param request Parametry zapytania
     * @return JSON obiekt gotowy do wysłania
     */
    QJsonObject buildRequestBody(const OllamaRequest& request) const;

    /**
     * @brief Parsuje odpowiedź JSON z Ollama API.
     * @param jsonData Surowe dane JSON
     * @return Sparsowana odpowiedź
     */
    OllamaResponse parseResponse(const QByteArray& jsonData, int statusCode) const;

    QNetworkAccessManager* m_networkManager; ///< Manager sieci Qt
    QString m_baseUrl;                       ///< URL serwera
    int m_timeoutMs;                         ///< Timeout w ms
};

} // namespace api
