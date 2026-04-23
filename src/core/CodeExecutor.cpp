#include "CodeExecutor.h"
#include "../utils/Logger.h"

#include <QDir>
#include <QFile>
#include <QTimer>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QThread>
#include <memory>

namespace core {

CodeExecutor::CodeExecutor(QObject* parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_isRunning(false)
    , m_pythonExecutable(findPythonExecutable())
{
}

CodeExecutor::~CodeExecutor() {
    cancelExecution();
}

QString CodeExecutor::findPythonExecutable() {
    QStringList candidates;
#ifdef Q_OS_WIN
    candidates << "python" << "python3" << "py";
#else
    candidates << "python3" << "python";
#endif
    for (const QString& candidate : candidates) {
        QProcess probe;
        probe.start(candidate, {"--version"});
        if (probe.waitForFinished(2000) && probe.exitCode() == 0) {
            return candidate;
        }
    }
    return {};
}

QString CodeExecutor::extractCode(const QString& llmResponse) const {
    QRegularExpression pyBlock("```python\\s*\\n([\\s\\S]*?)```",
                               QRegularExpression::CaseInsensitiveOption);
    auto match = pyBlock.match(llmResponse);
    if (match.hasMatch()) return match.captured(1).trimmed();

    QRegularExpression genericBlock("```\\s*\\n([\\s\\S]*?)```");
    match = genericBlock.match(llmResponse);
    if (match.hasMatch()) return match.captured(1).trimmed();

    QString trimmed = llmResponse.trimmed();
    if (trimmed.startsWith("```"))
        trimmed = trimmed.mid(trimmed.indexOf('\n') + 1);
    if (trimmed.endsWith("```"))
        trimmed = trimmed.left(trimmed.lastIndexOf("```")).trimmed();
    return trimmed;
}

void CodeExecutor::executeAsync(const QString& code,
                                std::function<void(ExecutionResult)> callback,
                                int timeoutMs)
{
    if (m_isRunning) {
        ExecutionResult err;
        err.success = false;
        err.stderr_output = "Inny kod jest juz wykonywany.";
        err.exitCode = -1;
        callback(err);
        return;
    }

    if (m_pythonExecutable.isEmpty()) {
        ExecutionResult err;
        err.success = false;
        err.stderr_output = "Nie znaleziono interpretera Python w systemie.\n"
                            "Zainstaluj Python 3 i upewnij sie, ze jest w PATH.";
        err.exitCode = -1;
        callback(err);
        return;
    }

    QString tempFile = saveToTempFile(code);
    if (tempFile.isEmpty()) {
        ExecutionResult err;
        err.success = false;
        err.stderr_output = "Nie mozna zapisac tymczasowego pliku kodu.";
        err.exitCode = -1;
        callback(err);
        return;
    }

    utils::Logger::instance().log("Wykonuje kod Python: " + tempFile);

    m_isRunning = true;

    // FIX: Use shared_ptr to hold accumulated output so lambdas
    // captured by value don't dangle after executeAsync() returns.
    auto stdoutBuf = std::make_shared<QString>();
    auto stderrBuf = std::make_shared<QString>();

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(QDir::tempPath());

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, [this, stdoutBuf]() {
        if (!m_process) return;
        QString chunk = QString::fromUtf8(m_process->readAllStandardOutput());
        *stdoutBuf += chunk;
        emit outputLine(chunk);
    });

    connect(m_process, &QProcess::readyReadStandardError,
            this, [this, stderrBuf]() {
        if (!m_process) return;
        *stderrBuf += QString::fromUtf8(m_process->readAllStandardError());
    });

    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->start(timeoutMs);

    connect(timer, &QTimer::timeout,
            this, [this, callback, stdoutBuf, stderrBuf, timer]() {
        timer->deleteLater();
        if (m_process && m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
        ExecutionResult result;
        result.success = false;
        result.stdout_output = *stdoutBuf;
        result.stderr_output = *stderrBuf + "\n[TIMEOUT] Przekroczono limit czasu wykonania.";
        result.exitCode = -1;
        m_isRunning = false;
        emit executionFinished(result);
        callback(result);
    });

    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, callback, tempFile, timer, stdoutBuf, stderrBuf]
            (int exitCode, QProcess::ExitStatus status) {
        timer->stop();
        timer->deleteLater();

        if (m_process) {
            *stdoutBuf += QString::fromUtf8(m_process->readAllStandardOutput());
            *stderrBuf += QString::fromUtf8(m_process->readAllStandardError());
        }

        ExecutionResult result;
        result.exitCode = exitCode;
        result.stdout_output = *stdoutBuf;
        result.stderr_output = *stderrBuf;
        result.success = (exitCode == 0 && status == QProcess::NormalExit);
        result.generatedImagePath = extractGeneratedFilePath(*stdoutBuf);

        m_isRunning = false;
        QFile::remove(tempFile);

        utils::Logger::instance().log(
            QString("Kod zakonczony. Exit: %1").arg(exitCode));

        if (m_process) {
            m_process->deleteLater();
            m_process = nullptr;
        }

        emit executionFinished(result);
        callback(result);
    });

    m_process->start(m_pythonExecutable, {tempFile});

    if (!m_process->waitForStarted(3000)) {
        timer->stop();
        timer->deleteLater();
        ExecutionResult err;
        err.success = false;
        err.stderr_output = "Nie mozna uruchomic interpretera Python: " + m_pythonExecutable;
        err.exitCode = -1;
        m_isRunning = false;
        QFile::remove(tempFile);
        m_process->deleteLater();
        m_process = nullptr;
        callback(err);
    }
}

void CodeExecutor::setPythonExecutable(const QString& path) {
    if (path.isEmpty()) {
        m_pythonExecutable = findPythonExecutable();
    } else {
        m_pythonExecutable = path;
    }
    utils::Logger::instance().log("Python interpreter ustawiony na: " + m_pythonExecutable);
}

void CodeExecutor::cancelExecution() {
    if (m_process && m_isRunning) {
        m_process->kill();
        m_process->waitForFinished(2000);
        m_isRunning = false;
    }
}

bool CodeExecutor::isRunning() const {
    return m_isRunning;
}

void CodeExecutor::installDependencies(const QStringList& packages,
                                       std::function<void(bool, QString)> callback)
{
    if (m_pythonExecutable.isEmpty()) {
        callback(false, "Python nie jest dostepny.");
        return;
    }
    QStringList args = {"-m", "pip", "install"};
    args << packages;
    QProcess* pip = new QProcess(this);
    connect(pip, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [pip, callback](int exitCode, QProcess::ExitStatus) {
        QString out = QString::fromUtf8(pip->readAllStandardOutput())
                    + QString::fromUtf8(pip->readAllStandardError());
        callback(exitCode == 0, out);
        pip->deleteLater();
    });
    pip->start(m_pythonExecutable, args);
}

QString CodeExecutor::saveToTempFile(const QString& code) const {
    QString path = QDir::tempPath() + "/jpo_gen_"
                 + QString::number(QCoreApplication::applicationPid()) + ".py";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return {};
    file.write(code.toUtf8());
    file.close();
    return path;
}

QString CodeExecutor::extractGeneratedFilePath(const QString& output) const {
    QRegularExpression re(R"(WYKRES_ZAPISANY:\s*(.+?)[\r\n])",
                          QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(output);
    if (match.hasMatch()) return match.captured(1).trimmed();

    QRegularExpression imgRe(R"(([^\s'"]+\.(png|jpg|jpeg|pdf|svg)))",
                             QRegularExpression::CaseInsensitiveOption);
    auto imgMatch = imgRe.match(output);
    if (imgMatch.hasMatch()) {
        QString candidate = imgMatch.captured(1);
        if (QFile::exists(QDir::tempPath() + "/" + candidate))
            return QDir::tempPath() + "/" + candidate;
        if (QFile::exists(candidate))
            return candidate;
    }
    return {};
}

} // namespace core
