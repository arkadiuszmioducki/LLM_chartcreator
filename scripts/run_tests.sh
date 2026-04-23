#!/bin/bash
# =============================================================
# run_tests.sh — Skrypt uruchomienia testów jednostkowych
# =============================================================

set -e

echo "=== LLM chart creator — Testy jednostkowe ==="

BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Katalog build/ nie istnieje. Uruchom najpierw: ./scripts/build.sh Debug"
    exit 1
fi

cd "$BUILD_DIR"

if [ ! -f "LLM_chartcreator_tests" ]; then
    echo "Plik testów nie znaleziony. Kompilacja..."
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    make LLM_chartcreator_tests -j"$(nproc)"
fi

echo ""
echo "--- Uruchamianie testów ---"
./LLM_chartcreator_tests --gtest_color=yes

echo ""
echo "--- CTest ---"
ctest --output-on-failure
