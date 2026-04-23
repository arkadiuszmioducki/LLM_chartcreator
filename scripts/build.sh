#!/bin/bash
# =============================================================
# build.sh — Skrypt kompilacji projektu LLM chart creator
# =============================================================

set -e

echo "=== LLM chart creator — Kompilacja ==="

# Check dependencies
if ! command -v cmake &> /dev/null; then
    echo "BŁĄD: CMake nie znaleziony. Zainstaluj: sudo apt install cmake"
    exit 1
fi

if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
    echo "UWAGA: qmake nie znaleziony. Upewnij się, że Qt6 jest zainstalowane."
fi

BUILD_DIR="build"
BUILD_TYPE="${1:-Release}"

echo "Typ budowania: $BUILD_TYPE"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

make -j"$(nproc)"

echo ""
echo "=== Kompilacja zakończona pomyślnie ==="
echo "Uruchom aplikację: ./build/LLM_chartcreator"
echo "Uruchom testy:     ./build/LLM_chartcreator_tests"
