# Findings

## Supplied material (2026-11-09)

The initial attachment exceeded the executor's 32 MiB transfer limit. The user
provided a Drive link; the authenticated Drive fetch returned a downloadable
file reference. Downloaded 246,307,989 bytes and verified every ZIP member CRC.
No private URL is stored in this repository.

- Archive SHA256: `cce6ecd0e62d23de6795acf28e6edfe21d209bb0f04604e008e44994e528ba3b`.
- ZIP entries: 25,558, including directories and AppleDouble metadata.
- Extracted regular files: 9,316, excluding `__MACOSX` and eight framework symlinks.
  The original archive is retained intact. Extracted copy is for analysis; it is
  not a launchable reconstruction of the bundle's links/permissions.
- Formats: 7,732 PNG, 698 SPR, 393 XML, 318 WAV, 43 OGG, 70 PAL,
  three TTF, one DAT, plus libraries, headers and bundle metadata.
- Main executable: `Dungeons of Dredmor.app/Contents/MacOS/dredmor`.
- Executable SHA256: `a8718ac188f932ef5b965e637805c446681db7af8a609a5f44069ec5c6e1973d`.
- Architecture: thin i386 Mach-O; 27,235 symbol-table entries including duplicates.
- Eight Mach-O files: executable, five dylibs, SDL and SDL_ttf frameworks. Some libraries also have
  x86_64 slices. The game executable itself has no arm64 or x86_64 slice.
- Dependencies include Cocoa, Carbon, OpenGL, OpenAL, SDL, SDL_ttf, expat,
  zlib, libstdc++, libSystem, Objective-C and Steam API.
- Base data and `expansion`, `expansion2`, `expansion3` directories are present.
  Presence does not establish expansion completeness or gameplay scope.
- Info.plist reports CFBundleVersion `1.0`, which is not the patch version.
  The executable's startup version formatter is **1.1.4**: the format string
  at `0x21de18` is referenced at `0xb6abc` in the function starting `0xb69b0`.
  Immediately before the call at `0xb6add`, cdecl stack arguments at offsets
  4, 8 and 12 are populated with 1, 1 and 4 respectively. Confidence: high
  for the embedded patch version, based on static instruction analysis; not
  a captured full-game startup. Additional suffix/mod state remains unverified.

## Verified original routine

`__Z8distanceiiii` at virtual address `0x000f0360`, returning at `0x000f0384`.
The named function subtracts x and y coordinates, squares each with wrapping
32-bit arithmetic, sums with wrapping arithmetic, converts the signed sum to
single precision, takes a single-precision square root, and truncates to int.
Invalid float-to-int conversions produce INT32_MIN on the original SSE path.

`scripts/verify_distance.py` loads the user's actual binary segments in Unicorn
2.1.4, supplies cdecl arguments and a stack, and executes those instructions
without imported-function stubs. It compares returns against the compiled C++
implementation. Fixed seed 0xD0ED; eight directed cases, 4,096 normal randomized
cases, and 4,096 full-int32 randomized cases: **8,200 cases, zero mismatches**.
Confidence: high for this function, under default floating-point rounding.
This is isolated execution of the original routine, not original gameplay.
Raw measurements remain outside source control in analysis/distance.json.

## Room/asset evidence

The supplied base `game/rooms.xml` parses with Python ElementTree and contains
191 room definitions. The first is Starting Room, 9 by 16 cells. It includes
custom blockers, conditional decorations and vending machines. The importer
accepts only a limited symbol set, rejects unknown symbols, and preserves visual
rows independently from a provisional collision grid. It is not a full database
implementation.

`hero_idle_d.spr` has a 9-byte SPR header, one 64x64 frame, a two-byte delay,
768-byte RGB palette and 4,096 indices; total 4,875 bytes. The decoder exports
the first frame, assuming palette index zero is transparent. Trailer-bearing
files are rejected. This assumption is not yet checked against original rendering.
The basic tileset is 384x640; preview crops are provisional.

## Tools and public projects

- GameFrame checkout: `c30e3fc95c748a8ec44e3989faafc4220895cbab`.
  Read its AGENTS.md, re-setup skill, environment guide and setup script.
  Ran setup check and install; created external home and Dredmor project using
  documented commands. Integrated through scripts/gameframe.sh, without copying
  its generated project or game files into this repository.
- Ghidra 12.1.4 archive downloaded and SHA256 checked against GitHub release
  digest `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db`.
  Python ZIP extraction did not preserve executable modes; relevant modes fixed.
  Host Java 21 is a JRE, lacking javac. Ghidra cannot start with it. Attempts to
  obtain a full JDK failed (package indexes unavailable, Adoptium API 403,
  GitHub release repeated 504). Use the working Unicorn/Capstone fallback.
- GameFrame `tests/run.sh --ghidra`: **407 passed, 28 failed**. Failures include
  absent xxd, image/catalog dependency, Frida export lookup and Ghidra/JDK.
  Pillow subsequently installed; broad suite not rerun because Ghidra/JDK and
  unrelated missing utilities remain unresolved. REA not configured.
- Open Dredmor checkout: `58cc5fd9342993bfad57d07a409e1158601b52f4`.
  Godot/GDScript engine, GPLv3 license text. Requires an original installation.
  Reviewed its project structure and asset/room handling as public reference.
  No code or assets copied from it; no license selected for this repository yet.

## Execution strategy

Linux cannot natively launch this macOS application. Isolated i386 routines can
be measured in Unicorn where inputs, memory and dependencies can be established.
Full gameplay reference needs a suitable legacy Mac environment or another
user-supplied supported edition. Modern Apple Silicon cannot directly launch
the supplied 32-bit executable. The new AppKit target needs a real Mac with
Xcode tools for compilation, packaging and actual smoke testing.
