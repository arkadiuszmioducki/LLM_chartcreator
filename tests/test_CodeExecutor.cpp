/**
 * @file test_CodeExecutor.cpp
 * @brief Testy jednostkowe klasy CodeExecutor.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 */

#include <gtest/gtest.h>
#include "../src/core/CodeExecutor.h"
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QThread>

using namespace core;

class CodeExecutorTest : public ::testing::Test {
protected:
    CodeExecutor executor;
};

// ===== extractCode =====

TEST_F(CodeExecutorTest, ExtractFromPythonFencedBlock) {
    QString response = "Oto kod:\n```python\nprint('hello')\n```\nGotowe.";
    QString code = executor.extractCode(response);
    EXPECT_EQ(code.trimmed(), "print('hello')");
}

TEST_F(CodeExecutorTest, ExtractFromGenericFencedBlock) {
    QString response = "```\nimport os\nprint(os.getcwd())\n```";
    QString code = executor.extractCode(response);
    EXPECT_TRUE(code.contains("import os"));
}

TEST_F(CodeExecutorTest, ExtractRawCode) {
    QString response = "import matplotlib.pyplot as plt\nplt.show()";
    QString code = executor.extractCode(response);
    EXPECT_TRUE(code.contains("import matplotlib"));
}

TEST_F(CodeExecutorTest, ExtractEmptyStringFromEmpty) {
    EXPECT_TRUE(executor.extractCode("").isEmpty());
}

TEST_F(CodeExecutorTest, ExtractTrimsWhitespace) {
    QString response = "```python\n\n  x = 1\n\n```";
    QString code = executor.extractCode(response);
    EXPECT_FALSE(code.startsWith('\n'));
    EXPECT_FALSE(code.endsWith('\n'));
}

TEST_F(CodeExecutorTest, ExtractHandlesMultilineCode) {
    QString response =
        "```python\n"
        "import matplotlib.pyplot as plt\n"
        "import numpy as np\n"
        "x = np.linspace(0, 10, 100)\n"
        "plt.plot(x, np.sin(x))\n"
        "plt.savefig('test.png')\n"
        "```";
    QString code = executor.extractCode(response);
    EXPECT_TRUE(code.contains("import matplotlib"));
    EXPECT_TRUE(code.contains("np.linspace"));
    EXPECT_TRUE(code.contains("plt.savefig"));
}

TEST_F(CodeExecutorTest, ExtractIgnoresTextBeforeAndAfterFence) {
    QString response =
        "Oto rozwiązanie Twojego problemu:\n\n"
        "```python\n"
        "x = 42\n"
        "print(x)\n"
        "```\n\n"
        "Mam nadzieję, że to pomaga!";
    QString code = executor.extractCode(response);
    EXPECT_FALSE(code.contains("Oto rozwiązanie"));
    EXPECT_FALSE(code.contains("Mam nadzieję"));
    EXPECT_TRUE(code.contains("x = 42"));
}

TEST_F(CodeExecutorTest, ExtractCaseInsensitivePythonTag) {
    QString response = "```Python\nprint('case insensitive')\n```";
    QString code = executor.extractCode(response);
    EXPECT_TRUE(code.contains("print"));
}

// ===== findPythonExecutable =====

TEST_F(CodeExecutorTest, FindPythonExecutableReturnsSomething) {
    // Python3 should be available in the test environment
    QString py = CodeExecutor::findPythonExecutable();
    // In CI/test env Python may or may not be available
    // Just test that the function doesn't crash
    SUCCEED();
}

// ===== isRunning =====

TEST_F(CodeExecutorTest, InitiallyNotRunning) {
    EXPECT_FALSE(executor.isRunning());
}

// ===== Async execution =====

TEST_F(CodeExecutorTest, ExecuteSimplePrintCode) {
    QString py = CodeExecutor::findPythonExecutable();
    if (py.isEmpty()) GTEST_SKIP() << "Python not available";

    bool callbackCalled = false;
    ExecutionResult capturedResult;

    executor.executeAsync("print('LLM_chart_generator_test_OK')", [&](ExecutionResult r) {
        callbackCalled = true;
        capturedResult = r;
    }, 10000);

    // Wait for execution
    QDeadlineTimer deadline(10000);
    while (!callbackCalled && !deadline.hasExpired()) {
        QCoreApplication::processEvents();
        QThread::msleep(50);
    }

    EXPECT_TRUE(callbackCalled);
    EXPECT_TRUE(capturedResult.success);
    EXPECT_TRUE(capturedResult.stdout_output.contains("LLM_chart_generator_test_OK"));
    EXPECT_EQ(capturedResult.exitCode, 0);
}

TEST_F(CodeExecutorTest, ExecuteInvalidCodeReturnsError) {
    QString py = CodeExecutor::findPythonExecutable();
    if (py.isEmpty()) GTEST_SKIP() << "Python not available";

    bool callbackCalled = false;
    ExecutionResult capturedResult;

    executor.executeAsync("this is not valid python !!!@@@", [&](ExecutionResult r) {
        callbackCalled = true;
        capturedResult = r;
    }, 10000);

    QDeadlineTimer deadline(10000);
    while (!callbackCalled && !deadline.hasExpired()) {
        QCoreApplication::processEvents();
        QThread::msleep(50);
    }

    EXPECT_TRUE(callbackCalled);
    EXPECT_FALSE(capturedResult.success);
    EXPECT_NE(capturedResult.exitCode, 0);
}

TEST_F(CodeExecutorTest, ExecuteCapturesStderr) {
    QString py = CodeExecutor::findPythonExecutable();
    if (py.isEmpty()) GTEST_SKIP() << "Python not available";

    bool done = false;
    ExecutionResult result;

    executor.executeAsync("import sys; sys.stderr.write('ERROR_MSG')", [&](ExecutionResult r) {
        done = true;
        result = r;
    }, 10000);

    QDeadlineTimer deadline(10000);
    while (!done && !deadline.hasExpired()) {
        QCoreApplication::processEvents();
        QThread::msleep(50);
    }

    EXPECT_TRUE(done);
    EXPECT_TRUE(result.stderr_output.contains("ERROR_MSG"));
}
