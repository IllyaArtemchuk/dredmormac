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
- Ghidra: /workspace/work/tools/ghidra_12.1.4_PUBLIC (needs full JDK)
- Logs: /workspace/work/gameframe-setup.log, gameframe-tests.log, dredmor-ghidra.log.

Use `PATH=/workspace/work/tools/game-frame/.venv/bin:$PATH scripts/build.sh`.
Run original comparison with that environment's Python, following README.
Do not execute the entire original binary as a Linux program.

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

Continue from the ordered tasks. Source has no selected distribution license;
do not import GPL code or publish releases without resolving project licensing.
