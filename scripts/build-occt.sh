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
mapfile -t flags < "$FLAGS_FILE"
nice -n 10 cmake -S "$SRC" -B "$BUILD" -G Ninja "${flags[@]}" "-DCMAKE_INSTALL_PREFIX=$PREFIX"
/usr/bin/time -f 'OCCT build wall=%E max-rss-kb=%M' nice -n 10 cmake --build "$BUILD" --parallel 3
nice -n 10 cmake --install "$BUILD"
