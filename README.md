# JPO Code Generator

> Generator kodu Python z automatycznym wykonaniem, wykorzystujący lokalny model językowy (Ollama + gemma4:e2b)  
> Projekt zaliczeniowy — Języki Programowania Obiektowego 2025/2026

---

## Opis projektu

Aplikacja desktopowa (C++ / Qt6) umożliwiająca generację i automatyczne wykonanie kodu Python za pomocą lokalnego modelu językowego uruchomionego przez **Ollama**.

### Funkcjonalności

- **Generator wykresów** — podaj dane i parametry, model generuje kod Python z matplotlib, który jest automatycznie wykonywany; wynik wyświetlany w GUI
- **Diagnostyka sieci** — generacja i auto-wykonanie kodu do pingowania hostów (cross-platform: Windows / Linux)
- **Podgląd i edycja kodu** — wygenerowany kod można przejrzeć i zmodyfikować przed wykonaniem
- **Wyniki wykonania** — stdout/stderr, podgląd wygenerowanego wykresu bezpośrednio w aplikacji
- **Konfiguracja** — zmiana URL serwera Ollama i nazwy modelu w ustawieniach
- **Obsługa błędów** — timeout, brak serwera, błędny kod Python — wszystko obsługiwane z informacją dla użytkownika
- **Wielowątkowość** — generacja (QNetworkAccessManager async) i wykonanie (QProcess async) nie blokują GUI

---

## Wymagania

### Systemowe
- **C++17** lub nowszy
- **Qt 6.4+** (Widgets, Network, Concurrent)
- **CMake 3.16+**
- **Python 3** (z pip) — do wykonania generowanego kodu
- **Ollama** — lokalny serwer LLM

### Python (biblioteki do wykresów)
```
pip install matplotlib numpy pandas
```

### Ollama + model
```bash
# Instalacja Ollama: https://ollama.com/
ollama pull gemma4:e2b
ollama serve
```

---

## Kompilacja i uruchomienie

### Linux / macOS
```bash
git clone https://github.com/arkadiuszmioducki/LLM_chartcreator.git
cd LLM_chartcreator

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

./LLM_chartcreator
```

### Windows (Qt Creator)
1. Otwórz Qt Creator
2. **File → Open File or Project** → wybierz `CMakeLists.txt`
3. Skonfiguruj kit Qt 6 (MinGW lub MSVC)
4. **Build → Build Project** (`Ctrl+B`)
5. **Run** (`Ctrl+R`)

### Uruchomienie z Qt Creator (wszystkie platformy)
1. Otwórz Qt Creator → `CMakeLists.txt`
2. Wybierz kit z Qt 6.4+
3. `Ctrl+B` → `Ctrl+R`

---

## Uruchomienie testów jednostkowych

### CMake / CLI
```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make LLM_chartcreator_tests -j4
ctest --output-on-failure
# lub bezpośrednio:
./LLM_chartcreator_tests
```

### Qt Creator
1. Otwórz projekt
2. Zmień target na `LLM_chartcreator_tests`
3. `Ctrl+R`

---

## Generowanie dokumentacji Doxygen

```bash
cd docs
doxygen Doxyfile
# Otwórz w przeglądarce:
xdg-open doxygen/html/index.html    # Linux
start doxygen/html/index.html       # Windows
```

---

## Struktura projektu

```
JPO_Project/
├── CMakeLists.txt
├── README.md
├── requirements.txt
├── src/
│   ├── main.cpp
│   ├── api/
│   │   ├── OllamaClient.h      # Klient REST API Ollama
│   │   └── OllamaClient.cpp
│   ├── core/
│   │   ├── PromptBuilder.h     # Budowanie system/user promptów
│   │   ├── PromptBuilder.cpp
│   │   ├── CodeExecutor.h      # Ekstrakcja i wykonanie kodu Python
│   │   └── CodeExecutor.cpp
│   ├── gui/
│   │   ├── MainWindow.h        # Główne okno aplikacji
│   │   ├── MainWindow.cpp
│   │   ├── SettingsDialog.h    # Dialog ustawień
│   │   ├── SettingsDialog.cpp
│   │   ├── OutputWidget.h
│   │   └── OutputWidget.cpp
│   └── utils/
│       ├── Logger.h            # Singleton loggera
│       ├── Logger.cpp
│       ├── FileUtils.h         # Operacje na plikach
│       └── FileUtils.cpp
├── tests/
│   ├── test_main.cpp
│   ├── test_OllamaClient.cpp
│   ├── test_PromptBuilder.cpp
│   ├── test_CodeExecutor.cpp
│   └── test_FileUtils.cpp
└── docs/
    └── Doxyfile
```

---

## Architektura

```
GUI (Qt Widgets)
    │
    ├── MainWindow ──── SettingsDialog
    │
    ├── api::OllamaClient      (async REST → Ollama /api/generate)
    │        │
    │   core::PromptBuilder    (system prompt + user prompt)
    │
    └── core::CodeExecutor     (extrakacja kodu, QProcess Python)
             │
        utils::Logger, utils::FileUtils
```

### Przepływ danych

```
Użytkownik podaje parametry
        ↓
PromptBuilder buduje prompt
        ↓
OllamaClient wysyła POST /api/generate (async)
        ↓
Model zwraca kod Python
        ↓
CodeExecutor::extractCode() wyodrębnia kod
        ↓
GUI pokazuje kod w zakładce "Wygenerowany Kod"
        ↓
Użytkownik klika "Wykonaj" (lub auto-wykonanie)
        ↓
CodeExecutor::executeAsync() → QProcess python3
        ↓
Stdout/stderr → zakładka "Wyniki"
Wykres PNG → podgląd w GUI
```

---

## Obsługa wyjątków / błędów

| Sytuacja | Zachowanie |
|---|---|
| Ollama niedostępna | Komunikat z instrukcją uruchomienia |
| Timeout zapytania | Informacja o przekroczeniu czasu |
| Python niedostępny | Komunikat z instrukcją instalacji |
| Błąd składni kodu | Stderr wyświetlony w zakładce Wyniki |
| Timeout wykonania kodu | Automatyczne zakończenie procesu po 60s |
| Brak danych wejściowych | Walidacja z ostrzeżeniem przed wysłaniem |

---

## Konfiguracja modelu

W pliku ustawień (lub przez **Ustawienia** w GUI):

| Parametr | Domyślna wartość |
|---|---|
| URL serwera | `http://localhost:11434` |
| Model | `gemma4:e2b` |
| Temperatura | `0.15` (niskie = deterministyczny kod) |
| Timeout API | `60 s` |
| Timeout wykonania | `60 s` |

---

## Autor
Arkadiusz Mioducki and Kamil Płóciennik
Projekt zaliczeniowy JPO 2025/2026  
Język: C++17 | Framework: Qt 6 | LLM: Ollama (gemma4:e2b)
