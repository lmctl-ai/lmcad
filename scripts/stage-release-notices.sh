#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="${1:?dist directory}"
OCCT_PREFIX="${2:?OCCT install prefix}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
if [[ "$BUILD_DIR" != /* ]]; then BUILD_DIR="$ROOT/$BUILD_DIR"; fi
mkdir -p "$DIST"
DIST_DIR="$(cd "$DIST" && pwd)"
cp "$ROOT/LICENSE" "$DIST/LICENSE"
cp "$ROOT/THIRD_PARTY_NOTICES.md" "$DIST/THIRD_PARTY_NOTICES"
cp "$ROOT/licenses/Apache-2.0.txt" "$DIST/Apache-2.0.txt"
cp "$ROOT/licenses/build123d-NOTICE.txt" "$DIST/build123d-NOTICE.txt"
cp "$ROOT/licenses/PyVista-MIT.txt" "$DIST/PyVista-MIT.txt"
cp "$ROOT/licenses/GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt" "$DIST/GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt"
cp "$ROOT/licenses/LINUX-RELINKING.md" "$DIST/OCCT-RELINKING.md"
cp "$OCCT_PREFIX/share/doc/opencascade/LICENSE_LGPL_21.txt" "$DIST/OCCT-LICENSE_LGPL_21.txt"
cp "$OCCT_PREFIX/share/doc/opencascade/OCCT_LGPL_EXCEPTION.txt" "$DIST/OCCT-LGPL-EXCEPTION.txt"
cp /usr/share/common-licenses/LGPL-2.1 "$DIST/LGPL-2.1.txt"
cp /usr/share/common-licenses/GPL-3 "$DIST/GPL-3.txt"
gcc_major="$(g++ -dumpfullversion -dumpversion | cut -d. -f1)"
cp "/usr/share/doc/gcc-${gcc_major}-base/copyright" "$DIST/GCC-TOOLCHAIN-COPYRIGHT"
cp /usr/share/doc/libc6/copyright "$DIST/GLIBC-COPYRIGHT"

binary="$(find "$DIST" -maxdepth 1 -type f -name 'lmcad-linux-*' ! -name '*-relink-objects.tar.gz' -print -quit)"
[[ -n "$binary" ]] || { echo "missing staged lmcad-linux-<arch> binary in $DIST" >&2; exit 1; }
build_objects="$BUILD_DIR/CMakeFiles/lmcad.dir/cpp"
[[ -d "$build_objects" ]] || { echo "missing application objects: $build_objects" >&2; exit 1; }
mapfile -d '' objects < <(find "$build_objects" -maxdepth 1 -type f -name '*.o' -print0 | sort -z)
((${#objects[@]} == 6)) || { echo "expected 6 lmcad link objects; found ${#objects[@]}" >&2; exit 1; }
relative_objects=()
for object in "${objects[@]}"; do relative_objects+=("${object#"$BUILD_DIR"/}"); done
archive="$DIST/$(basename "$binary")-relink-objects.tar.gz"
tar -czf "$archive" -C "$BUILD_DIR" "${relative_objects[@]}" -C "$DIST_DIR" LICENSE THIRD_PARTY_NOTICES OCCT-RELINKING.md
