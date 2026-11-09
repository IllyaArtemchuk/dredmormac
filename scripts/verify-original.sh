#!/usr/bin/env bash
# Compare the port with the user's original binary/data; retain private reports externally.
set -euo pipefail
[[ $# == 2 ]] || { echo 'Usage: scripts/verify-original.sh DATA_ROOT PRIVATE_ANALYSIS_DIR' >&2; exit 2; }
data_root="$(cd "$1" && pwd)"
mkdir -p "$2"
report_root="$(cd "$2" && pwd)"
cd "$(dirname "$0")/.."
python_cmd=python3
[[ ! -x .venv/bin/python ]] || python_cmd="$PWD/.venv/bin/python"
"$python_cmd" scripts/verify_distance.py "$data_root/dredmor" build/dredmor-distance --out "$report_root/distance.json"
"$python_cmd" scripts/verify_random.py "$data_root/dredmor" build/dredmor-rng --out "$report_root/random.json"
"$python_cmd" scripts/verify_stats.py "$data_root/dredmor" build/dredmor-stats --out "$report_root/stats.json"
"$python_cmd" scripts/verify_character.py "$data_root/dredmor" build/dredmor-character --out "$report_root/character.json"
"$python_cmd" scripts/verify_catalog.py "$data_root" build/dredmor-data --out "$report_root/catalog-base.json"
"$python_cmd" scripts/verify_catalog.py "$data_root" build/dredmor-data --expansions --out "$report_root/catalog-all.json"
