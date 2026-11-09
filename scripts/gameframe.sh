#!/usr/bin/env bash
# Use one external GameFrame checkout; retain all project source in this repo.
set -euo pipefail
: "${GAMEFRAME_ROOT:?Set GAMEFRAME_ROOT to your game-frame checkout}"
exec "$GAMEFRAME_ROOT/gameframe" "$@"
