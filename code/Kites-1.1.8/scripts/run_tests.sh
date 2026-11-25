#!/usr/bin/env bash
# Builds and runs the whole Google Test suite (Linux / macOS).
# Usage:  scripts/run_tests.sh            # all tests
#         scripts/run_tests.sh Rvv        # only tests whose name matches "Rvv"
# Needs CMake 3.24+, Ninja, a C++20 compiler and Qt 6 (set QT_DIR, e.g. ~/Qt/6.12.0/gcc_64).
set -euo pipefail
cd "$(dirname "$0")/.."
QT_DIR="${QT_DIR:-}"
cmake -S . -B build/test -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON -DENABLE_COVERAGE=OFF -DDISABLE_GUI=ON -DLOG_PANEL=OFF ${QT_DIR:+-DCMAKE_PREFIX_PATH="$QT_DIR"}
cmake --build build/test --target kites_test --parallel
if [ $# -gt 0 ]; then
  ctest --test-dir build/test --output-on-failure -R "$1"
else
  ctest --test-dir build/test --output-on-failure
fi
