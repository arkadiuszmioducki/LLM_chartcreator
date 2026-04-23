/**
 * @file test_OllamaClient.cpp
 * @brief Testy jednostkowe klasy OllamaClient.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 */

#include <gtest/gtest.h>
#include "../src/api/OllamaClient.h"
#include <QDeadlineTimer>
#include <QThread>
#include <QCoreApplication>

using namespace api;

class OllamaClientTest : public ::testing::Test {
protected:
    OllamaClient client{"http://localhost:11434"};
};

TEST_F(OllamaClientTest, DefaultBaseUrlIsSet) {
    EXPECT_EQ(client.baseUrl(), "http://localhost:11434");
}

TEST_F(OllamaClientTest, SetBaseUrlWorks) {
    client.setBaseUrl("http://localhost:9999");
    EXPECT_EQ(client.baseUrl(), "http://localhost:9999");
}

TEST_F(OllamaClientTest, SetTimeoutDoesNotCrash) {
    client.setTimeout(5000);
    SUCCEED();
}

TEST_F(OllamaClientTest, IsServerAvailableReturnsBoolWithoutCrash) {
    // Server may or may not be running in test env - just ensure no crash
    bool result = client.isServerAvailable();
    EXPECT_TRUE(result == true || result == false);
}

TEST_F(OllamaClientTest, SendRequestToUnavailableServerCallsCallback) {
    OllamaClient badClient("http://localhost:1");  // Port that's never open
    badClient.setTimeout(2000);

    bool callbackCalled = false;
    OllamaResponse capturedResponse;

    OllamaRequest req;
    req.model = "test";
    req.systemPrompt = "test";
    req.userPrompt = "test";

    badClient.sendRequest(req, [&](OllamaResponse resp) {
        callbackCalled = true;
        capturedResponse = resp;
    });

    // Wait for callback
    QDeadlineTimer deadline(5000);
    while (!callbackCalled && !deadline.hasExpired()) {
        QCoreApplication::processEvents();
        QThread::msleep(50);
    }

    EXPECT_TRUE(callbackCalled);
    EXPECT_FALSE(capturedResponse.success);
    EXPECT_FALSE(capturedResponse.errorMsg.isEmpty());
}

TEST_F(OllamaClientTest, FetchModelsCallsCallback) {
    OllamaClient badClient("http://localhost:1");
    badClient.setTimeout(2000);

    bool called = false;
    badClient.fetchAvailableModels([&](QStringList models, QString err) {
        called = true;
        // Either we get models or an error - both are valid
        Q_UNUSED(models); Q_UNUSED(err);
    });

    QDeadlineTimer deadline(5000);
    while (!called && !deadline.hasExpired()) {
        QCoreApplication::processEvents();
        QThread::msleep(50);
    }

    EXPECT_TRUE(called);
}
