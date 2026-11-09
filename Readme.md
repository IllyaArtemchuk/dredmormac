# DredmorMac

Native Apple Silicon macOS reimplementation in progress. This checkpoint is an
analysis foundation and provisional room preview, **not a complete game**.
Original game files are required and are never distributed with the source.

The portable C++17 core runs deterministic room replays. A native AppKit target
loads an original tileset and an exported original hero sprite. The distance
routine has been checked against 8,200 executions of the supplied i386 binary;
movement, room rendering and the macOS frontend have not been compared with
original gameplay. See [status](docs/STATUS.md) and [deviations](docs/DEVIATIONS.md).

## Build and verify

Requires a C++17 compiler and Python 3.11+. Setup installs project-local CMake
and Unicorn; no paid service or original game download is involved.

```sh
scripts/setup.sh
export PATH="$PWD/.venv/bin:$PATH"
scripts/build.sh
build/dredmor-replay tests/room.txt 'EES.WW'
```

The room text format is an internal harness format: `#` blocks, `.` is floor,
and exactly one `@` marks the initial player. Actions are `N S W E .` without
spaces. Each line of output is a state after the corresponding action, preceded
by the initial state. Blocked moves consume no turn under the provisional policy.

## Use your original installation

Keep data outside the repository. For the supplied Mac app, DATA_ROOT is its
`Contents/MacOS` directory, where game/, tilesets/ and sprites/ reside.

```sh
python3 scripts/inventory.py '/private/path/Dungeons of Dredmor.app' \
  --out '/private/analysis/inventory.json'
python3 scripts/prepare_scene.py '/private/path/Dungeons of Dredmor.app/Contents/MacOS' \
  --out '/private/derived/starting-room'
build/dredmor-replay '/private/derived/starting-room/room.txt' 'NNEES.WW'
.venv/bin/python scripts/verify_distance.py \
  '/private/path/Dungeons of Dredmor.app/Contents/MacOS/dredmor' \
  build/dredmor-distance --out '/private/analysis/distance.json'
```

The comparison runs the original function in an i386 emulator, not the entire
game. It requires the named i386 Mach-O executable from the supplied edition.
Reports include paths and original metadata; keep them private and outside git.
Generated scenes contain machine-local absolute paths; regenerate them on a Mac.

## Native macOS handoff

On an actual Mac, install Xcode command line tools, Python 3.11+ and CMake,
then prepare a scene locally using the commands above.

```sh
scripts/build-macos.sh
build/macos-arm64/DredmorMac.app/Contents/MacOS/DredmorMac \
  '/private/derived/starting-room/scene.json'
```

Arrow keys move; `.` waits. Closing the window quits. This target is written but
has **not been compiled or smoke-tested on a Mac**. No SDK cross-compilation,
signing, notarization or full-game compatibility is claimed. The original
32-bit app needs a legacy macOS environment for full reference gameplay.

## GameFrame

Use one separate checkout of https://github.com/tanepiper/game-frame. Read its
setup instructions, inspect scripts before running them, and keep its home and
analysis project outside this repository. `scripts/gameframe.sh` uses the
`GAMEFRAME_ROOT` environment variable; `GAMEFRAME_HOME` selects external storage.
The existing source repository remains authoritative; do not replace it with
GameFrame's generated project. Tool versions, failures and evidence are in
[findings](docs/FINDINGS.md); continuation instructions are in [resume](docs/RESUME.md).
