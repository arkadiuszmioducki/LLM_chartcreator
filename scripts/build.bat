@echo off
REM =============================================================
REM build.bat — Skrypt kompilacji dla Windows
REM =============================================================

echo === LLM chart creator — Kompilacja (Windows) ===

SET BUILD_DIR=build
SET BUILD_TYPE=Release

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -G "MinGW Makefiles"

mingw32-make -j%NUMBER_OF_PROCESSORS%

echo.
echo === Kompilacja zakonczona ===
echo Uruchom aplikacje: .\build\LLM_chartcreator.exe
echo Uruchom testy:     .\build\LLM_chartcreator_tests.exe
pause
