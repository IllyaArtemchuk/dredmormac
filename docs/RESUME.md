# Resume

Read AGENTS.md, STATUS.md, FINDINGS.md and DEVIATIONS.md. This is not a complete
game. Do not overwrite source with the GameFrame scaffold.

Current cloud paths (private material, never commit):
- Repository: /workspace/dredmormac
- Original archive: /workspace/work/Archive.zip
- GameFrame: /workspace/work/tools/game-frame
- GameFrame home: /workspace/work/gameframe-home
- Scaffold project: /workspace/work/dredmor-analysis (tooling only)
- Original app: /workspace/work/gameframe-home/games/dredmor/game/Dungeons of Dredmor.app
- Analysis and derived: sibling analysis/ and derived/ in games/dredmor/.
- Tool environment: /workspace/work/tools/game-frame/.venv
- Ghidra: /workspace/work/tools/ghidra_12.1.4_PUBLIC
- Working Java compiler home: /workspace/work/tools/java-21-reproduced
- Logs: /workspace/work/gameframe-setup.log, gameframe-tests.log, dredmor-ghidra.log.

Use `PATH=/workspace/work/tools/game-frame/.venv/bin:$PATH scripts/build.sh`.
Run original comparison with that environment's Python, following README.
Do not execute the entire original binary as a Linux program.

The repository now also has its own .venv. Use
`PATH="$PWD/.venv/bin:$PATH" scripts/build.sh`, then
`scripts/verify-original.sh DATA_ROOT PRIVATE_ANALYSIS_DIR` to run distance,
RNG, stat-fragment, full-function character fixtures and database count checks.
The C++ build additionally requires Expat development headers/library.

Ghidra is working. The installed Java runtime contains jdk.compiler; only its
javac launcher was absent. Reproduce the local compiler home with
`python3 scripts/java-compiler-home.py /usr/lib/jvm/java-21-openjdk-amd64 NEW_LOCAL_HOME`.
Set JAVA_HOME, GHIDRA_INSTALL_DIR and GAMEFRAME_ROOT, then run
`scripts/analyze-original.sh ORIGINAL_BINARY PRIVATE_ANALYSIS_DIR`.
This uses the saved project named dredmor; GameFrame's ghidra-run.sh assumes a
project named re, so do not point it at this project unchanged.

Reports: random.json, stats.json, character.json, catalog-base.json,
catalog-all.json and recovered-functions.txt in the private analysis directory.
The final file is decompiler output and must remain outside tracked source.

Fresh environment: clone repository, run scripts/setup.sh, obtain the user's
archive privately, verify recorded SHA256, inspect ZIP names/types, and extract
regular app files outside git. Preserve the original ZIP for legacy Mac use;
the analysis extraction deliberately skips framework links and executable modes.
Use scripts/inventory.py to reproduce inventory and scripts/prepare_scene.py for
the scene. Do not put private transfer URLs in source, logs to share, or commits.

Mac handoff: install Python 3.11+, CMake and Xcode command line tools; use the
same original data root, prepare scene locally so absolute asset paths are valid,
run scripts/build-macos.sh, then launch the app binary with scene.json. Record
actual build/launch evidence; fix compile errors before declaring Mac support.
Modern macOS cannot run the original i386 game, so native port testing and
original gameplay capture may require different machines.

`.github/workflows/native-macos.yml` runs on implementation-branch pushes and
workflow_dispatch. Actual run 42585141286 passed all thirteen checks on standard
macos-15 arm64, including native combat and relocated/ad-hoc-signed package smoke.
The development app artifact contains no original assets. Direct animated SPR
loading now has local decoder tests and 698-file/3,633-frame palette comparisons;
its native synthetic animation smoke passes. Save/Load menu controls are added;
their native atomic roundtrip smoke passes. Room-placement kernel matches
69,632 original executions; generation-flow.txt is a private export of
WriteTemplate, GenLevelContent1, Branch::GetTileID and branch XML callback. Original asset
visual comparison and full original gameplay parity remain pending.

Continue from the ordered tasks. Source has no selected distribution license;
do not import GPL code or publish releases without resolving project licensing.
