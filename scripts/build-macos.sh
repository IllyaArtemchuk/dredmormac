#!/usr/bin/env bash
# Build an arm64 AppKit application on a Mac with Xcode command line tools.
set -euo pipefail
cd "$(dirname "$0")/.."
[[ "$(uname -s)" == Darwin ]] || { echo 'Run on macOS with Xcode command line tools.' >&2; exit 1; }
xcrun --find clang++ >/dev/null
cmake -S . -B build/macos-arm64 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build/macos-arm64 --parallel 2
ctest --test-dir build/macos-arm64 --output-on-failure
file build/macos-arm64/DredmorMac.app/Contents/MacOS/DredmorMac
echo 'Launch: build/macos-arm64/DredmorMac.app/Contents/MacOS/DredmorMac /absolute/path/to/scene.json'
