#include "OllamaClient.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QUrl>
#include <QEventLoop>

namespace api {

OllamaClient::OllamaClient(const QString& baseUrl, QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_baseUrl(baseUrl)
    , m_timeoutMs(60000)
{
}

void OllamaClient::setBaseUrl(const QString& url) {
    m_baseUrl = url;
}

QString OllamaClient::baseUrl() const {
    return m_baseUrl;
}

void OllamaClient::setTimeout(int ms) {
    m_timeoutMs = ms;
}

bool OllamaClient::isServerAvailable() const {
    QNetworkAccessManager mgr;

    // FIX: avoid "most vexing parse" - use setUrl() instead of constructor argument
    QNetworkRequest req;
    req.setUrl(QUrl(m_baseUrl));
    req.setTransferTimeout(3000);

    auto* reply = mgr.get(req);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    bool ok = (reply->error() == QNetworkReply::NoError ||
               reply->error() == QNetworkReply::ContentNotFoundError);
    reply->deleteLater();
    return ok;
}

void OllamaClient::sendRequest(const OllamaRequest& request,
                               std::function<void(OllamaResponse)> callback)
{
    QNetworkRequest netRequest;
    netRequest.setUrl(QUrl(m_baseUrl + "/api/generate"));
    netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    netRequest.setTransferTimeout(m_timeoutMs);

    QJsonDocument doc(buildRequestBody(request));
    QByteArray body = doc.toJson(QJsonDocument::Compact);

    QNetworkReply* reply = m_networkManager->post(netRequest, body);

    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->start(m_timeoutMs);

    connect(timer, &QTimer::timeout, this, [reply, callback, timer]() {
        timer->stop();
        reply->abort();
        OllamaResponse errResp;
        errResp.success = false;
        errResp.errorMsg = "Przekroczono limit czasu oczekiwania na odpowiedź serwera.";
        errResp.statusCode = 0;
        callback(errResp);
        timer->deleteLater();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, timer]() {
        timer->stop();
        timer->deleteLater();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        OllamaResponse response;

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            response = parseResponse(data, statusCode);
        } else {
            response.success = false;
            response.statusCode = statusCode;

            switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                response.errorMsg = "Odmowa polaczenia. Sprawdz czy Ollama jest uruchomiona na " + m_baseUrl;
                break;
            case QNetworkReply::HostNotFoundError:
                response.errorMsg = "Nie znaleziono hosta. Sprawdz adres serwera: " + m_baseUrl;
                break;
            case QNetworkReply::TimeoutError:
                response.errorMsg = "Przekroczono limit czasu. Model moze byc przeciazony.";
                break;
            case QNetworkReply::OperationCanceledError:
                response.errorMsg = "Operacja anulowana (timeout).";
                break;
            default:
                response.errorMsg = "Blad sieciowy: " + reply->errorString();
                break;
            }
            emit networkError(response.errorMsg);
        }

        emit responseReceived(response);
        callback(response);
        reply->deleteLater();
    });
}

void OllamaClient::fetchAvailableModels(std::function<void(QStringList, QString)> callback) {
    QNetworkRequest netRequest;
    netRequest.setUrl(QUrl(m_baseUrl + "/api/tags"));
    netRequest.setTransferTimeout(5000);

    QNetworkReply* reply = m_networkManager->get(netRequest);

    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        if (reply->error() != QNetworkReply::NoError) {
            callback({}, reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray models = doc.object()["models"].toArray();
        QStringList modelNames;
        for (const auto& m : models) {
            modelNames << m.toObject()["name"].toString();
        }
        callback(modelNames, "");
        reply->deleteLater();
    });
}

QJsonObject OllamaClient::buildRequestBody(const OllamaRequest& request) const {
    QJsonObject body;
    body["model"] = request.model;
    body["system"] = request.systemPrompt;
    body["prompt"] = request.userPrompt;
    body["stream"] = request.stream;

    QJsonObject options;
    options["temperature"] = request.temperature;
    body["options"] = options;

    return body;
}

OllamaResponse OllamaClient::parseResponse(const QByteArray& jsonData, int statusCode) const {
    OllamaResponse response;
    response.statusCode = statusCode;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        response.success = false;
        response.errorMsg = "Blad parsowania JSON: " + parseError.errorString();
        return response;
    }

    QJsonObject obj = doc.object();

    if (obj.contains("error")) {
        response.success = false;
        response.errorMsg = obj["error"].toString();
        return response;
    }

    response.success = true;
    response.response = obj["response"].toString();
    return response;
}

} // namespace api
