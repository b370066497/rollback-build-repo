#!/usr/bin/env bash
# Build a patched EmulatorJS FBNeo core with rollback support.
#
# Run on Linux (WSL / the EmulatorJS build devcontainer). Requires:
#   git build-essential pkgconf python3 zip p7zip-full libsdl2-dev jq wget curl gpg
#
# Strategy: pre-seed a patched EmulatorJS/RetroArch checkout at the exact path
# the official build script uses, commit the patch (so build.sh's `git pull`
# cannot revert it), then run the official build.sh for the fbneo core.
#
# Output: build/output/fbneo-wasm.data  (drop into public/ejs/data/cores/)

set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$HERE/work"
EMSDK_VERSION="3.1.74"
RA_BRANCH="v1.22.2"
CORE="${EJS_CORE:-fbneo}"

mkdir -p "$WORK"
cd "$WORK"

# 1. emsdk --------------------------------------------------------------
if [ ! -d emsdk ]; then
  git clone https://github.com/emscripten-core/emsdk.git
fi
cd emsdk
./emsdk install "$EMSDK_VERSION"
./emsdk activate "$EMSDK_VERSION"
# shellcheck disable=SC1091
source ./emsdk_env.sh
cd "$WORK"

# 2. build repo ---------------------------------------------------------
if [ ! -d build ]; then
  git clone --depth 1 https://github.com/EmulatorJS/build.git build
fi

# The core declares a minimum EmulatorJS version (from build.json). Lower it to
# the project's EmulatorJS version (4.2.3) so the loader accepts the core.
if [ -f "$WORK/build/build.json" ] && command -v jq >/dev/null 2>&1; then
  jq '.minimumEJSVersion = "4.2.3"' "$WORK/build/build.json" > "$WORK/build/build.json.tmp" \
    && mv "$WORK/build/build.json.tmp" "$WORK/build/build.json"
  echo "build.json minimumEJSVersion -> 4.2.3"
fi

# 3. pre-seed patched RetroArch where build.sh expects it ----------------
RA="$WORK/build/compile/RetroArch"
mkdir -p "$WORK/build/compile"
if [ ! -d "$RA" ]; then
  git clone --depth 1 -b "$RA_BRANCH" https://github.com/EmulatorJS/RetroArch.git "$RA"
fi
if [ ! -d "$RA/EmulatorJS" ]; then
  git clone --depth 1 https://github.com/EmulatorJS/EmulatorJS.git "$RA/EmulatorJS"
fi

STATE_FILE="$(grep -rln "save_state_info" "$RA" --include='*.c' | head -n1 || true)"
# retroarch.c is the file that defines the static emscripten_frame_count.
STEP_FILE="$(grep -rln "emscripten_frame_count" "$RA" --include='*.c' | head -n1 || true)"
if [ -z "$STATE_FILE" ] || [ -z "$STEP_FILE" ]; then
  echo "Could not locate patch targets." >&2
  echo "state file: $STATE_FILE" >&2
  echo "step file:  $STEP_FILE" >&2
  exit 1
fi
echo "state file: $STATE_FILE"
echo "step file:  $STEP_FILE"

if ! grep -q "ejs_save_state" "$STATE_FILE"; then
  cat "$HERE/rollback_state.c" >> "$STATE_FILE"
fi
if ! grep -q "ejs_step_frame" "$STEP_FILE"; then
  cat "$HERE/rollback_step.c" >> "$STEP_FILE"
fi

MAKEFILE="$RA/Makefile.emulatorjs"
grep -q "_ejs_save_state" "$MAKEFILE" || sed -i \
  's/_get_current_frame_count,_ejs_set_keyboard_enabled/_get_current_frame_count,_ejs_set_keyboard_enabled,_ejs_state_size,_ejs_save_state,_ejs_load_state,_ejs_set_frame_input,_ejs_step_frame,_ejs_get_frame/' \
  "$MAKEFILE"

# commit so build.sh's `git pull` keeps our changes
git -C "$RA" add -A
git -C "$RA" -c user.email=build@local -c user.name=build commit -m "EJS rollback wrapper" || true

# 4. build with the official script -------------------------------------
cd "$WORK/build"
source "$WORK/emsdk/emsdk_env.sh"
bash build.sh -c="$CORE"

echo "== done =="
CORE_OUT="$WORK/build/output/${CORE}-wasm.data"
if [ ! -f "$CORE_OUT" ]; then
  echo "ERROR: expected core not produced: $CORE_OUT" >&2
  echo "See $WORK/build/output/logs/ for the failing step." >&2
  exit 1
fi
echo "core: $CORE_OUT"
echo "copy it to: public/ejs/data/cores/${CORE}-wasm.data"
