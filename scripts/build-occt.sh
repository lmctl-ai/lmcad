#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${OCCT_SRC:-$ROOT/build/occt-src}"
BUILD="${OCCT_BUILD:-$ROOT/build/occt-build}"
PREFIX="${OCCT_PREFIX:-$ROOT/build/occt-prefix}"
FLAGS_FILE="$ROOT/.github/occt-cmake-flags.txt"
if [[ ! -d "$SRC/.git" ]]; then
  rm -rf "$SRC"
  git clone --depth 1 --branch V8_0_1 https://github.com/Open-Cascade-SAS/OCCT.git "$SRC"
fi
# bash 3.2 (macOS) has no mapfile; git-bash has no /usr/bin/time; MSYS `nice`
# is absent, so run unniced there.
flags=()
while IFS= read -r line; do flags+=("$line"); done < "$FLAGS_FILE"
NICE=(nice -n 10)
if [[ "$(uname -s)" == MINGW* || "$(uname -s)" == MSYS* ]]; then NICE=(); fi
"${NICE[@]}" cmake -S "$SRC" -B "$BUILD" -G Ninja "${flags[@]}" "-DCMAKE_INSTALL_PREFIX=$PREFIX"
# BSD time (macOS) takes no -f; git-bash has no /usr/bin/time at all.
if /usr/bin/time -f %e true >/dev/null 2>&1; then
  /usr/bin/time -f 'OCCT build wall=%E max-rss-kb=%M' "${NICE[@]}" cmake --build "$BUILD" --parallel 3
else
  "${NICE[@]}" cmake --build "$BUILD" --parallel 3
fi
"${NICE[@]}" cmake --install "$BUILD"
