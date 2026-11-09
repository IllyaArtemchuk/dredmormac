#!/usr/bin/env bash
# Import once, then export selected routines from the saved Ghidra project.
set -euo pipefail
[[ $# == 2 ]] || { echo 'Usage: scripts/analyze-original.sh ORIGINAL_BINARY PRIVATE_ANALYSIS_DIR' >&2; exit 2; }
: "${GHIDRA_INSTALL_DIR:?Set GHIDRA_INSTALL_DIR}"
: "${GAMEFRAME_ROOT:?Set GAMEFRAME_ROOT}"
: "${JAVA_HOME:?Set JAVA_HOME to a JDK or the prepared compiler home}"
binary="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
mkdir -p "$2/ghidra-project"
report_root="$(cd "$2" && pwd)"
project_scripts="$(cd "$(dirname "$0")" && pwd)/ghidra"
args=(-import "$binary" -analysisTimeoutPerFile 180)
if [[ -f "$report_root/ghidra-project/dredmor.gpr" ]]; then
  args=(-process "$(basename "$binary")" -noanalysis)
fi
"$GHIDRA_INSTALL_DIR/support/analyzeHeadless" "$report_root/ghidra-project" dredmor \
  "${args[@]}" -scriptPath "$project_scripts;$GAMEFRAME_ROOT/tools/ghidra" \
  -postScript RepairAllocatorReturns.java \
  -postScript ExportAddressBatch.java "$report_root/recovered-functions.txt" \
  0xf0360 0x190800 0x190870 0x190a00 0x1218de 0xc93d0 0xc9490 0xcea00
