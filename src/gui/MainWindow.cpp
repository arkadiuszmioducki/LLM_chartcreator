#include "MainWindow.h"
#include "SettingsDialog.h"
#include "../utils/Logger.h"
#include "../utils/FileUtils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QPixmap>
#include <QFont>
#include <QThread>
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

namespace gui {

// ============================================================
//  Konstruktor
// ============================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ollamaClient(std::make_unique<api::OllamaClient>())
    , m_codeExecutor(std::make_unique<core::CodeExecutor>())
    , m_promptBuilder(std::make_unique<core::PromptBuilder>())
    , m_ollamaUrl("http://localhost:11434")
    , m_modelName("gemma4:e2b")
    , m_pythonPath("")
{
    setWindowTitle("LLM chart creator — Generator wykresow Python z LLM");
    setMinimumSize(1000, 720);
    resize(1250, 820);

    loadSettings();
    setupUi();

    // Apply saved python path to executor
    if (!m_pythonPath.isEmpty())
        m_codeExecutor->setPythonExecutable(m_pythonPath);

    connect(m_codeExecutor.get(), &core::CodeExecutor::outputLine,
            this, &MainWindow::onOutputLine, Qt::QueuedConnection);

    utils::Logger::instance().log("Aplikacja uruchomiona");
    debugLog("APP", "LLM chart creator uruchomiony. Model: " + m_modelName
             + " | Ollama: " + m_ollamaUrl);
}

// ============================================================
//  UI setup
// ============================================================

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Top bar
    QHBoxLayout* topBar = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("LLM chart creator", this);
    QFont tf = titleLabel->font(); tf.setPointSize(14); tf.setBold(true);
    titleLabel->setFont(tf);

    m_serverStatusLabel = new QLabel("● Serwer: nieznany", this);
    m_serverStatusLabel->setStyleSheet("color: gray; font-weight: bold;");

    QPushButton* checkBtn = new QPushButton("Sprawdz Ollama", this);
    checkBtn->setFixedWidth(130);
    connect(checkBtn, &QPushButton::clicked, this, &MainWindow::onCheckServerClicked);

    QPushButton* settingsBtn = new QPushButton("Ustawienia", this);
    settingsBtn->setFixedWidth(100);
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    m_toggleDebugBtn = new QPushButton("Debug", this);
    m_toggleDebugBtn->setFixedWidth(80);
    m_toggleDebugBtn->setCheckable(true);
    connect(m_toggleDebugBtn, &QPushButton::clicked, this, &MainWindow::onToggleDebugPanel);

    topBar->addWidget(titleLabel);
    topBar->addStretch();
    topBar->addWidget(m_serverStatusLabel);
    topBar->addWidget(checkBtn);
    topBar->addWidget(settingsBtn);
    topBar->addWidget(m_toggleDebugBtn);
    mainLayout->addLayout(topBar);

    // Tabs: only Chart, Code, Output
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createChartTab(),  "Generator Wykresow");
    m_tabWidget->addTab(createCodeTab(),   "Wygenerowany Kod");
    m_tabWidget->addTab(createOutputTab(), "Wyniki Wykonania");
    mainLayout->addWidget(m_tabWidget, 1);

    // Progress
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setTextVisible(false);
    mainLayout->addWidget(m_progressBar);

    // Action bar
    QHBoxLayout* actionBar = new QHBoxLayout();

    auto makeBtn = [&](const QString& text, const QString& bg, const QString& hover) {
        QPushButton* b = new QPushButton(text, this);
        b->setFixedHeight(38);
        b->setStyleSheet(QString(
            "QPushButton{background:%1;color:white;font-weight:bold;border-radius:5px;font-size:13px;}"
            "QPushButton:hover{background:%2;}"
            "QPushButton:disabled{opacity:0.5;}").arg(bg, hover));
        return b;
    };

    m_generateBtn = makeBtn("Generuj kod",  "#2563eb", "#1d4ed8");
    m_executeBtn  = makeBtn("Wykonaj kod",  "#16a34a", "#15803d");
    m_cancelBtn   = makeBtn("Anuluj",       "#dc2626", "#b91c1c");
    m_clearBtn    = new QPushButton("Wyczysc", this);
    m_clearBtn->setFixedHeight(38);

    m_executeBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);

    connect(m_generateBtn, &QPushButton::clicked, this, &MainWindow::onGenerateClicked);
    connect(m_executeBtn,  &QPushButton::clicked, this, &MainWindow::onExecuteClicked);
    connect(m_cancelBtn,   &QPushButton::clicked, this, &MainWindow::onCancelClicked);
    connect(m_clearBtn,    &QPushButton::clicked, this, &MainWindow::onClearClicked);

    actionBar->addWidget(m_generateBtn);
    actionBar->addWidget(m_executeBtn);
    actionBar->addWidget(m_cancelBtn);
    actionBar->addWidget(m_clearBtn);
    actionBar->addStretch();
    mainLayout->addLayout(actionBar);

    // Debug dock
    addDockWidget(Qt::BottomDockWidgetArea, createDebugDock());
    m_debugDock->hide();

    statusBar()->showMessage("Gotowy. Upewnij sie, ze Ollama jest uruchomiona.");
}

// ============================================================
//  Zakladka: Generator Wykresow
// ============================================================

QWidget* MainWindow::createChartTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);

    // Left panel: parameters
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFixedWidth(380);

    QWidget* paramsW = new QWidget();
    QFormLayout* form = new QFormLayout(paramsW);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setSpacing(8);

    m_chartTypeCombo = new QComboBox();
    m_chartTypeCombo->addItems({"Line", "Bar", "Scatter", "Histogram", "Pie"});
    form->addRow("Typ wykresu:", m_chartTypeCombo);

    m_chartTitleEdit = new QLineEdit();
    m_chartTitleEdit->setPlaceholderText("np. Sprzedaz miesięczna 2024");
    form->addRow("Tytul:", m_chartTitleEdit);

    m_xLabelEdit = new QLineEdit();
    m_xLabelEdit->setPlaceholderText("np. Miesiac");
    form->addRow("Etykieta osi X:", m_xLabelEdit);

    m_yLabelEdit = new QLineEdit();
    m_yLabelEdit->setPlaceholderText("np. Wartosc [PLN]");
    form->addRow("Etykieta osi Y:", m_yLabelEdit);

    m_colorSchemeCombo = new QComboBox();
    m_colorSchemeCombo->addItems({"tab10","viridis","plasma","Set2","Set3","Blues","Reds","Greens"});
    form->addRow("Schemat kolorow:", m_colorSchemeCombo);

    QHBoxLayout* checkRow = new QHBoxLayout();
    m_showGridCheck   = new QCheckBox("Siatka");   m_showGridCheck->setChecked(true);
    m_showLegendCheck = new QCheckBox("Legenda");  m_showLegendCheck->setChecked(true);
    checkRow->addWidget(m_showGridCheck);
    checkRow->addWidget(m_showLegendCheck);
    form->addRow("Opcje:", checkRow);

    QHBoxLayout* sizeRow = new QHBoxLayout();
    m_figWidthSpin  = new QSpinBox(); m_figWidthSpin->setRange(4,30);  m_figWidthSpin->setValue(10);
    m_figHeightSpin = new QSpinBox(); m_figHeightSpin->setRange(3,20); m_figHeightSpin->setValue(6);
    sizeRow->addWidget(new QLabel("Szer:")); sizeRow->addWidget(m_figWidthSpin);
    sizeRow->addWidget(new QLabel("Wys:"));  sizeRow->addWidget(m_figHeightSpin);
    form->addRow("Rozmiar (cale):", sizeRow);

    m_outputFormatCombo = new QComboBox();
    m_outputFormatCombo->addItems({"png","pdf","svg","jpg"});
    form->addRow("Format wyjsciowy:", m_outputFormatCombo);

    // Default path: Documents/LLM_chart_creator/
    QString docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString defaultDir = docsPath + "/LLM_chart_creator";
    QDir().mkpath(defaultDir);
    m_outputPathEdit = new QLineEdit(defaultDir + "/wykres.png");
    form->addRow("Sciezka zapisu:", m_outputPathEdit);

    // Auto-sync extension when format changes
    connect(m_outputFormatCombo, &QComboBox::currentTextChanged,
            this, [this](const QString& fmt) {
        QString path = m_outputPathEdit->text();
        int lastSlash = qMax(path.lastIndexOf('/'), path.lastIndexOf('\\'));
        int dotPos = path.lastIndexOf('.');
        if (dotPos > lastSlash) path = path.left(dotPos + 1) + fmt;
        else path = path + "." + fmt;
        m_outputPathEdit->setText(path);
    });

    scroll->setWidget(paramsW);
    layout->addWidget(scroll);

    // Right panel: data + extra
    QVBoxLayout* rightL = new QVBoxLayout();

    QGroupBox* dataGroup = new QGroupBox("Dane wejsciowe (CSV / JSON / opis slowny)");
    QVBoxLayout* dataL = new QVBoxLayout(dataGroup);
    m_dataEdit = new QTextEdit();
    m_dataEdit->setPlaceholderText(
        "Przykladowe formaty:\n\n"
        "CSV (x i y w osobnych wierszach):\n"
        "x: 1,2,3,4,5\n"
        "y: 10,25,15,30,20\n\n"
        "JSON:\n"
        "{\"labels\":[\"Sty\",\"Lut\",\"Mar\"],\"values\":[100,150,120]}\n\n"
        "Opis slowny:\n"
        "Dane sprzedazy 2024: Q1=120k, Q2=145k, Q3=98k, Q4=210k"
    );
    m_dataEdit->setMinimumHeight(160);
    m_loadDataBtn = new QPushButton("Zaladuj z pliku CSV/TXT");
    connect(m_loadDataBtn, &QPushButton::clicked, this, &MainWindow::onLoadDataFileClicked);
    dataL->addWidget(m_dataEdit);
    dataL->addWidget(m_loadDataBtn);
    rightL->addWidget(dataGroup);

    QGroupBox* extraGroup = new QGroupBox(
        "Dodatkowe instrukcje dla modelu — np. styl, adnotacje, kolory warunkowe (opcjonalne)");
    QVBoxLayout* extraL = new QVBoxLayout(extraGroup);
    m_additionalInfoEdit = new QTextEdit();
    m_additionalInfoEdit->setPlaceholderText(
        "Przyklady:\n"
        "- Uzyj stylu ggplot\n"
        "- Dodaj adnotacje wartosci na wierzcholkach slupkow\n"
        "- Slupki powyzej 100 pokoloruj na czerwono\n"
        "- Dodaj linie trendu\n"
        "- Uzyj skali logarytmicznej na osi Y");
    m_additionalInfoEdit->setFixedHeight(120);
    extraL->addWidget(m_additionalInfoEdit);
    rightL->addWidget(extraGroup);

    layout->addLayout(rightL, 1);
    return tab;
}

// ============================================================
//  Zakladka: Wygenerowany Kod
// ============================================================

QWidget* MainWindow::createCodeTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* btnRow = new QHBoxLayout();
    m_saveCodeBtn = new QPushButton("Zapisz kod do pliku .py");
    connect(m_saveCodeBtn, &QPushButton::clicked, this, &MainWindow::onSaveCodeClicked);
    QLabel* hint = new QLabel("Mozesz edytowac kod przed wykonaniem");
    hint->setStyleSheet("color:#6b7280;font-style:italic;");
    btnRow->addWidget(m_saveCodeBtn);
    btnRow->addStretch();
    btnRow->addWidget(hint);
    layout->addLayout(btnRow);

    m_codeEdit = new QPlainTextEdit();
    QFont f("Courier New", 10); f.setFixedPitch(true);
    m_codeEdit->setFont(f);
    m_codeEdit->setPlaceholderText("Tu pojawi sie wygenerowany kod Python...");
    layout->addWidget(m_codeEdit, 1);
    return tab;
}

// ============================================================
//  Zakladka: Wyniki Wykonania
// ============================================================

QWidget* MainWindow::createOutputTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QSplitter* splitter = new QSplitter(Qt::Vertical, tab);

    // Console output
    QWidget* consoleW = new QWidget();
    QVBoxLayout* consoleL = new QVBoxLayout(consoleW);
    consoleL->setContentsMargins(0,0,0,0);
    QLabel* lbl1 = new QLabel("Wyjscie skryptu Python:");
    lbl1->setStyleSheet("font-weight:bold;");
    consoleL->addWidget(lbl1);
    m_outputEdit = new QPlainTextEdit();
    m_outputEdit->setReadOnly(true);
    QFont cf("Courier New", 9); cf.setFixedPitch(true);
    m_outputEdit->setFont(cf);
    m_outputEdit->setStyleSheet("background:#1e1e1e;color:#d4d4d4;");
    consoleL->addWidget(m_outputEdit);
    splitter->addWidget(consoleW);

    // Image preview
    QWidget* imgW = new QWidget();
    QVBoxLayout* imgL = new QVBoxLayout(imgW);
    imgL->setContentsMargins(0,0,0,0);
    QHBoxLayout* imgBtnRow = new QHBoxLayout();
    QLabel* lbl2 = new QLabel("Wygenerowany wykres:");
    lbl2->setStyleSheet("font-weight:bold;");
    m_openImageBtn = new QPushButton("Otworz w przegladarce");
    m_openImageBtn->setEnabled(false);
    connect(m_openImageBtn, &QPushButton::clicked, this, &MainWindow::onOpenImageClicked);
    imgBtnRow->addWidget(lbl2);
    imgBtnRow->addStretch();
    imgBtnRow->addWidget(m_openImageBtn);
    imgL->addLayout(imgBtnRow);
    m_imageLabel = new QLabel("Tu pojawi sie wygenerowany wykres");
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background:#f3f4f6;border:1px solid #d1d5db;border-radius:4px;");
    m_imageLabel->setMinimumHeight(200);
    imgL->addWidget(m_imageLabel, 1);
    splitter->addWidget(imgW);

    layout->addWidget(splitter, 1);
    return tab;
}

// ============================================================
//  Panel debugowania
// ============================================================

QDockWidget* MainWindow::createDebugDock() {
    m_debugDock = new QDockWidget("Panel debugowania", this);
    m_debugDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    m_debugDock->setFeatures(QDockWidget::DockWidgetMovable |
                             QDockWidget::DockWidgetFloatable |
                             QDockWidget::DockWidgetClosable);

    QWidget* dockContent = new QWidget();
    QVBoxLayout* dockLayout = new QVBoxLayout(dockContent);
    dockLayout->setContentsMargins(4, 4, 4, 4);
    dockLayout->setSpacing(4);

    QHBoxLayout* dbgBtnRow = new QHBoxLayout();
    QLabel* dbgLabel = new QLabel("Log zapytan / odpowiedzi / zdarzen aplikacji:");
    dbgLabel->setStyleSheet("font-weight:bold;");
    QPushButton* clearDbgBtn = new QPushButton("Wyczysc log");
    clearDbgBtn->setFixedWidth(110);
    connect(clearDbgBtn, &QPushButton::clicked, this, &MainWindow::onClearDebugLog);
    dbgBtnRow->addWidget(dbgLabel);
    dbgBtnRow->addStretch();
    dbgBtnRow->addWidget(clearDbgBtn);
    dockLayout->addLayout(dbgBtnRow);

    m_debugEdit = new QPlainTextEdit();
    m_debugEdit->setReadOnly(true);
    QFont dbgFont("Courier New", 9); dbgFont.setFixedPitch(true);
    m_debugEdit->setFont(dbgFont);
    m_debugEdit->setStyleSheet("background:#0f172a;color:#94a3b8;");
    m_debugEdit->setMinimumHeight(160);
    dockLayout->addWidget(m_debugEdit);

    m_debugDock->setWidget(dockContent);

    // Sync toggle button state with dock visibility
    connect(m_debugDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        m_toggleDebugBtn->setChecked(visible);
    });

    return m_debugDock;
}

// ============================================================
//  Zbieranie parametrow
// ============================================================

core::ChartParameters MainWindow::collectChartParams() const {
    core::ChartParameters p;
    p.type        = core::PromptBuilder::stringToChartType(m_chartTypeCombo->currentText());
    p.title       = m_chartTitleEdit->text().trimmed();
    p.xLabel      = m_xLabelEdit->text().trimmed();
    p.yLabel      = m_yLabelEdit->text().trimmed();
    p.dataInput   = m_dataEdit->toPlainText().trimmed();
    p.colorScheme = m_colorSchemeCombo->currentText();
    p.showGrid    = m_showGridCheck->isChecked();
    p.showLegend  = m_showLegendCheck->isChecked();

    // Format from combo — always authoritative
    p.outputFormat = m_outputFormatCombo->currentText();

    // Strip existing extension from path and append selected format
    QString rawPath = m_outputPathEdit->text().trimmed();
    if (rawPath.isEmpty()) {
        QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        rawPath = docs + "/LLM_chart_creator/wykres";
    }
    int lastSlash = qMax(rawPath.lastIndexOf('/'), rawPath.lastIndexOf('\\'));
    int dotPos    = rawPath.lastIndexOf('.');
    if (dotPos > lastSlash) rawPath = rawPath.left(dotPos);
    p.outputPath = rawPath + "." + p.outputFormat;

    p.figWidth       = m_figWidthSpin->value();
    p.figHeight      = m_figHeightSpin->value();
    p.additionalInfo = m_additionalInfoEdit->toPlainText().trimmed();
    return p;
}

// ============================================================
//  Walidacja
// ============================================================

bool MainWindow::validateInputs() const {
    if (m_dataEdit->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(const_cast<MainWindow*>(this),
            "Brak danych", "Prosze podac dane wejsciowe dla wykresu.");
        return false;
    }
    return true;
}

// ============================================================
//  UI helpers
// ============================================================

void MainWindow::setBusy(bool busy) {
    m_generateBtn->setEnabled(!busy);
    m_executeBtn->setEnabled(!busy && !m_currentCode.isEmpty());
    m_cancelBtn->setEnabled(busy);
    m_progressBar->setVisible(busy);
}

void MainWindow::setStatus(const QString& message, bool isError) {
    statusBar()->showMessage(message);
    statusBar()->setStyleSheet(isError ? "QStatusBar{color:#dc2626;}" : "");
}

void MainWindow::displayCode(const QString& code) {
    m_currentCode = code;
    m_codeEdit->setPlainText(code);
    m_executeBtn->setEnabled(true);
    m_tabWidget->setCurrentIndex(1); // Code tab
}

void MainWindow::displayExecutionResult(const core::ExecutionResult& result) {
    m_outputEdit->setPlainText(result.stdout_output);
    if (!result.stderr_output.isEmpty())
        m_outputEdit->appendPlainText("\n--- STDERR ---\n" + result.stderr_output);

    if (result.success)
        setStatus("Kod wykonany pomyslnie");
    else
        setStatus("Blad wykonania kodu (exit: " + QString::number(result.exitCode) + ")", true);

    debugLog("EXEC", QString("Exit: %1 | Success: %2\nSTDOUT:\n%3%4")
             .arg(result.exitCode).arg(result.success)
             .arg(result.stdout_output.left(500))
             .arg(result.stderr_output.isEmpty() ? ""
                  : "\nSTDERR:\n" + result.stderr_output.left(300)));

    if (!result.generatedImagePath.isEmpty())
        displayImage(result.generatedImagePath);

    m_tabWidget->setCurrentIndex(2); // Output tab
}

void MainWindow::displayImage(const QString& imagePath) {
    QPixmap pix(imagePath);
    if (pix.isNull()) return;
    m_lastImagePath = imagePath;
    m_openImageBtn->setEnabled(true);
    QSize maxSize = m_imageLabel->size();
    if (maxSize.width() < 100) maxSize = QSize(700, 450);
    m_imageLabel->setPixmap(
        pix.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::debugLog(const QString& category, const QString& content) {
    if (!m_debugEdit) return;
    QString ts  = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString sep = QString(60, '-');
    QString entry = QString("[%1] [%2]\n%3\n%4\n").arg(ts, category, content, sep);
    m_debugEdit->appendPlainText(entry);
    m_debugEdit->ensureCursorVisible();
}

// ============================================================
//  Slots
// ============================================================

void MainWindow::onGenerateClicked() {
    if (!validateInputs()) return;

    m_ollamaClient->setBaseUrl(m_ollamaUrl);
    setBusy(true);
    setStatus("Generowanie kodu przez model jezykowy...");
    m_outputEdit->clear();

    auto params = collectChartParams();
    QString systemPrompt = m_promptBuilder->buildSystemPrompt(core::TaskType::ChartGeneration);
    QString userPrompt   = m_promptBuilder->buildUserPrompt(params);

    // --- Debug: log request ---
    QJsonObject reqJson;
    reqJson["model"]  = m_modelName;
    reqJson["system"] = systemPrompt;
    reqJson["prompt"] = userPrompt;
    reqJson["stream"] = false;
    debugLog("REQUEST", QJsonDocument(reqJson).toJson(QJsonDocument::Indented));

    api::OllamaRequest req;
    req.model        = m_modelName;
    req.systemPrompt = systemPrompt;
    req.userPrompt   = userPrompt;
    req.stream       = false;
    req.temperature  = 0.15;

    m_ollamaClient->sendRequest(req, [this](api::OllamaResponse resp) {
        QMetaObject::invokeMethod(this, [this, resp]() {
            setBusy(false);

            if (!resp.success) {
                debugLog("ERROR", "Blad API: " + resp.errorMsg
                         + "\nHTTP status: " + QString::number(resp.statusCode));
                setStatus("Blad: " + resp.errorMsg, true);
                QMessageBox::critical(this, "Blad komunikacji z Ollama",
                    resp.errorMsg + "\n\nSprawdz:\n"
                    "- Czy Ollama jest uruchomiona: ollama serve\n"
                    "- Czy model jest zainstalowany: ollama pull " + m_modelName);
                return;
            }

            // --- Debug: log response ---
            QJsonObject resJson;
            resJson["raw_response"] = resp.response.left(2000);
            resJson["http_status"]  = resp.statusCode;
            debugLog("RESPONSE", QJsonDocument(resJson).toJson(QJsonDocument::Indented));

            QString code = m_codeExecutor->extractCode(resp.response);
            if (code.isEmpty()) {
                debugLog("ERROR", "Model zwrocil pusta odpowiedz lub nie znaleziono kodu.");
                setStatus("Model zwrocil pusta odpowiedz", true);
                return;
            }

            debugLog("EXTRACTED CODE", code);
            displayCode(code);
            setStatus("Kod wygenerowany. Sprawdz zakladke 'Wygenerowany Kod' i kliknij Wykonaj.");
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onExecuteClicked() {
    QString code = m_codeEdit->toPlainText().trimmed();
    if (code.isEmpty()) {
        QMessageBox::warning(this, "Brak kodu", "Brak kodu do wykonania.");
        return;
    }

    setBusy(true);
    setStatus("Wykonywanie kodu Python...");
    m_outputEdit->clear();
    m_imageLabel->setText("Generowanie wykresu...");
    m_openImageBtn->setEnabled(false);

    debugLog("EXEC-START", "Uruchamianie skryptu Python...\n"
             "Interpreter: " + (m_pythonPath.isEmpty() ? "(auto)" : m_pythonPath));

    m_codeExecutor->executeAsync(code, [this](core::ExecutionResult result) {
        QMetaObject::invokeMethod(this, [this, result]() {
            setBusy(false);
            displayExecutionResult(result);
        }, Qt::QueuedConnection);
    }, 60000);
}

void MainWindow::onCancelClicked() {
    m_codeExecutor->cancelExecution();
    setBusy(false);
    debugLog("APP", "Operacja anulowana przez uzytkownika.");
    setStatus("Anulowano.");
}

void MainWindow::onSettingsClicked() {
    SettingsDialog dlg(m_ollamaUrl, m_modelName, m_pythonPath, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_ollamaUrl  = dlg.ollamaUrl();
        m_modelName  = dlg.modelName();
        m_pythonPath = dlg.pythonPath();
        m_ollamaClient->setBaseUrl(m_ollamaUrl);
        if (!m_pythonPath.isEmpty())
            m_codeExecutor->setPythonExecutable(m_pythonPath);
        else
            m_codeExecutor->setPythonExecutable(core::CodeExecutor::findPythonExecutable());
        saveSettings();
        debugLog("SETTINGS", "Ollama URL: " + m_ollamaUrl
                 + "\nModel: " + m_modelName
                 + "\nPython: " + (m_pythonPath.isEmpty() ? "(auto)" : m_pythonPath));
        setStatus("Ustawienia zapisane.");
    }
}

void MainWindow::onCheckServerClicked() {
    setStatus("Sprawdzanie serwera Ollama...");
    m_ollamaClient->setBaseUrl(m_ollamaUrl);
    bool ok = m_ollamaClient->isServerAvailable();
    if (ok) {
        m_serverStatusLabel->setText("Serwer: dostepny");
        m_serverStatusLabel->setStyleSheet("color:#16a34a;font-weight:bold;");
        setStatus("Serwer Ollama dostepny: " + m_ollamaUrl);
        debugLog("SERVER", "Serwer dostepny: " + m_ollamaUrl);
    } else {
        m_serverStatusLabel->setText("Serwer: niedostepny");
        m_serverStatusLabel->setStyleSheet("color:#dc2626;font-weight:bold;");
        setStatus("Serwer Ollama niedostepny. Uruchom: ollama serve", true);
        debugLog("ERROR", "Serwer niedostepny: " + m_ollamaUrl);
    }
}

void MainWindow::onClearClicked() {
    m_outputEdit->clear();
    m_codeEdit->clear();
    m_imageLabel->setText("Tu pojawi sie wygenerowany wykres");
    m_currentCode.clear();
    m_lastImagePath.clear();
    m_executeBtn->setEnabled(false);
    m_openImageBtn->setEnabled(false);
    setStatus("Wyczyszczono.");
}

void MainWindow::onLoadDataFileClicked() {
    QString path = QFileDialog::getOpenFileName(this, "Otworz plik danych",
        QDir::homePath(), "Pliki danych (*.csv *.txt *.json);;Wszystkie (*)");
    if (path.isEmpty()) return;
    bool ok;
    QString content = utils::FileUtils::readTextFile(path, &ok);
    if (!ok) {
        QMessageBox::warning(this, "Blad odczytu", "Nie mozna otworzyc pliku: " + path);
        return;
    }
    m_dataEdit->setPlainText(content);
    debugLog("FILE", "Zaladowano dane z: " + path
             + "\nRozmiar: " + QString::number(content.length()) + " znakow");
    setStatus("Zaladowano dane z: " + path);
}

void MainWindow::onSaveCodeClicked() {
    QString code = m_codeEdit->toPlainText();
    if (code.isEmpty()) {
        QMessageBox::information(this, "Brak kodu", "Nie ma kodu do zapisania.");
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, "Zapisz kod Python",
        QDir::homePath() + "/generated_chart.py", "Python (*.py);;Wszystkie (*)");
    if (path.isEmpty()) return;
    if (utils::FileUtils::writeTextFile(path, code))
        setStatus("Kod zapisany: " + path);
    else
        QMessageBox::critical(this, "Blad zapisu", "Nie mozna zapisac: " + path);
}

void MainWindow::onOpenImageClicked() {
    if (!m_lastImagePath.isEmpty() && utils::FileUtils::fileExists(m_lastImagePath))
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastImagePath));
}

void MainWindow::onOutputLine(const QString& line) {
    m_outputEdit->appendPlainText(line);
    m_outputEdit->ensureCursorVisible();
}

void MainWindow::onToggleDebugPanel() {
    if (m_debugDock->isVisible()) {
        m_debugDock->hide();
    } else {
        m_debugDock->show();
        m_debugDock->raise();
    }
}

void MainWindow::onClearDebugLog() {
    if (m_debugEdit) m_debugEdit->clear();
}

// ============================================================
//  Ustawienia
// ============================================================

void MainWindow::loadSettings() {
    QSettings s("LLM", "chartgenerator");
    m_ollamaUrl  = s.value("ollamaUrl",  "http://localhost:11434").toString();
    m_modelName  = s.value("modelName",  "gemma4:e2b").toString();
    m_pythonPath = s.value("pythonPath", "").toString();
}

void MainWindow::saveSettings() {
    QSettings s("LLM", "chartgenerator");
    s.setValue("ollamaUrl",  m_ollamaUrl);
    s.setValue("modelName",  m_modelName);
    s.setValue("pythonPath", m_pythonPath);
}

} // namespace gui
