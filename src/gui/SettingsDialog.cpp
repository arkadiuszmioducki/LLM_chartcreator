#include "SettingsDialog.h"
#include "../api/OllamaClient.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>

namespace gui {

SettingsDialog::SettingsDialog(const QString& currentUrl,
                               const QString& currentModel,
                               const QString& currentPython,
                               QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Ustawienia — LLM chart creator");
    setMinimumWidth(500);

    QVBoxLayout* layout = new QVBoxLayout(this);

    // --- Ollama connection ---
    QGroupBox* connGroup = new QGroupBox("Polaczenie z Ollama");
    QFormLayout* connForm = new QFormLayout(connGroup);

    m_urlEdit = new QLineEdit(currentUrl);
    m_urlEdit->setPlaceholderText("http://localhost:11434");
    connForm->addRow("URL serwera:", m_urlEdit);

    m_modelEdit = new QLineEdit(currentModel);
    m_modelEdit->setPlaceholderText("gemma4:e2b");
    connForm->addRow("Nazwa modelu:", m_modelEdit);

    QLabel* modelHint = new QLabel(
        "Zainstalowane modele: ollama list\n"
        "Pobierz model:        ollama pull gemma4:e2b");
    modelHint->setStyleSheet("color:#6b7280;font-size:11px;");
    connForm->addRow("", modelHint);

    QPushButton* testBtn = new QPushButton("Testuj polaczenie");
    connect(testBtn, &QPushButton::clicked, this, &SettingsDialog::onTestConnection);
    connForm->addRow("", testBtn);

    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    connForm->addRow("", m_statusLabel);

    layout->addWidget(connGroup);

    // --- Python interpreter ---
    QGroupBox* pyGroup = new QGroupBox("Interpreter Python");
    QFormLayout* pyForm = new QFormLayout(pyGroup);

    QHBoxLayout* pyRow = new QHBoxLayout();
    m_pythonEdit = new QLineEdit(currentPython);
    m_pythonEdit->setPlaceholderText("(puste = automatyczna detekcja: python3 / python)");
    QPushButton* browseBtn = new QPushButton("Przegladaj...");
    browseBtn->setFixedWidth(110);
    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowsePython);
    pyRow->addWidget(m_pythonEdit);
    pyRow->addWidget(browseBtn);
    pyForm->addRow("Sciezka do Python:", pyRow);

    QLabel* pyHint = new QLabel(
        "Pozostaw puste zeby uzyc systemowego Pythona.\n"
        "Przydatne gdy uzywasz virtualenv, conda lub\n"
        "Pythona spoza zmiennej PATH (np. z Microsoft Store).");
    pyHint->setStyleSheet("color:#6b7280;font-size:11px;");
    pyForm->addRow("", pyHint);

    layout->addWidget(pyGroup);

    // --- Buttons ---
    QDialogButtonBox* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btns);
}

QString SettingsDialog::ollamaUrl() const  { return m_urlEdit->text().trimmed(); }
QString SettingsDialog::modelName() const  { return m_modelEdit->text().trimmed(); }
QString SettingsDialog::pythonPath() const { return m_pythonEdit->text().trimmed(); }

void SettingsDialog::onTestConnection() {
    m_statusLabel->setText("Sprawdzanie...");
    m_statusLabel->setStyleSheet("color:#6b7280;");
    QApplication::processEvents();

    api::OllamaClient client(m_urlEdit->text().trimmed());
    client.setTimeout(4000);

    if (client.isServerAvailable()) {
        m_statusLabel->setText("Polaczenie udane!");
        m_statusLabel->setStyleSheet("color:#16a34a;font-weight:bold;");
    } else {
        m_statusLabel->setText("Brak polaczenia.\nSprawdz czy Ollama dziala: ollama serve");
        m_statusLabel->setStyleSheet("color:#dc2626;");
    }
}

void SettingsDialog::onBrowsePython() {
#ifdef Q_OS_WIN
    QString filter = "Python (python.exe python3.exe);;Wszystkie pliki (*.exe)";
    QString startDir = "C:/";
#else
    QString filter = "Python (python python3);;Wszystkie pliki (*)";
    QString startDir = "/usr/bin";
#endif
    QString path = QFileDialog::getOpenFileName(
        this, "Wybierz interpreter Python", startDir, filter);
    if (!path.isEmpty())
        m_pythonEdit->setText(path);
}

} // namespace gui
