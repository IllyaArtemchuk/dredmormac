#!/usr/bin/env bash
# Configure, build and run the portable checks. CMake 3.20+, C++17, Python 3.11+.
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPython3_EXECUTABLE="$(command -v python3)"
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
