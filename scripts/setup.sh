#!/usr/bin/env bash
# Project-local tools only; original game files and macOS SDK are supplied separately.
set -euo pipefail
cd "$(dirname "$0")/.."
python3 -m venv .venv
.venv/bin/python -m pip install 'cmake==4.4.4' 'unicorn==2.1.4'
echo 'Use: PATH="$PWD/.venv/bin:$PATH" scripts/build.sh'
