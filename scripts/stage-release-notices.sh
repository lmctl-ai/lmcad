#!/usr/bin/env bash
# Stage license/notice files beside a release binary.
# Usage: scripts/stage-release-notices.sh <dist-dir> <occt-prefix> [platform]
# platform: linux (default; full LGPL relink recipe + glibc/GCC copyright),
#           darwin (system libs only; no glibc/GCC runtime, no relink objects),
#           mingw  (MinGW-w64 static runtime; GPL-3 + GCC runtime exception,
#                   no glibc, no relink objects).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="${1:?dist directory}"
OCCT_PREFIX="${2:?OCCT install prefix}"
PLATFORM="${3:-linux}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
if [[ "$BUILD_DIR" != /* ]]; then BUILD_DIR="$ROOT/$BUILD_DIR"; fi
mkdir -p "$DIST"
DIST_DIR="$(cd "$DIST" && pwd)"

# Common to all platforms: project license/notices, reference licenses, OCCT.
cp "$ROOT/LICENSE" "$DIST/LICENSE"
cp "$ROOT/THIRD_PARTY_NOTICES.md" "$DIST/THIRD_PARTY_NOTICES"
cp "$ROOT/licenses/Apache-2.0.txt" "$DIST/Apache-2.0.txt"
cp "$ROOT/licenses/build123d-NOTICE.txt" "$DIST/build123d-NOTICE.txt"
cp "$ROOT/licenses/PyVista-MIT.txt" "$DIST/PyVista-MIT.txt"
# OCCT's Windows install keeps the licenses at the prefix root.
occt_doc="$OCCT_PREFIX/share/doc/opencascade"
[ -d "$occt_doc" ] || occt_doc="$OCCT_PREFIX"
cp "$occt_doc/LICENSE_LGPL_21.txt" "$DIST/OCCT-LICENSE_LGPL_21.txt"
cp "$occt_doc/OCCT_LGPL_EXCEPTION.txt" "$DIST/OCCT-LGPL-EXCEPTION.txt"
# OCCT is LGPL-2.1+exception everywhere; the license text ships in-repo so
# macOS/Windows staging does not depend on /usr/share/common-licenses.
cp "$ROOT/licenses/LGPL-2.1.txt" "$DIST/LGPL-2.1.txt"

if [[ "$PLATFORM" == linux ]]; then
  cp "$ROOT/licenses/GPL-3.txt" "$DIST/GPL-3.txt"
  cp "$ROOT/licenses/GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt" "$DIST/GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt"
  cp "$ROOT/licenses/LINUX-RELINKING.md" "$DIST/OCCT-RELINKING.md"
  gcc_major="$(g++ -dumpfullversion -dumpversion | cut -d. -f1)"
  cp "/usr/share/doc/gcc-${gcc_major}-base/copyright" "$DIST/GCC-TOOLCHAIN-COPYRIGHT"
  cp /usr/share/doc/libc6/copyright "$DIST/GLIBC-COPYRIGHT"

  binary="$(find "$DIST" -maxdepth 1 -type f -name 'lmcad-linux-*' ! -name '*-relink-objects.tar.gz' -print -quit)"
  [[ -n "$binary" ]] || { echo "missing staged lmcad-linux-<arch> binary in $DIST" >&2; exit 1; }
  build_objects="$BUILD_DIR/CMakeFiles/lmcad.dir/cpp"
  [[ -d "$build_objects" ]] || { echo "missing application objects: $build_objects" >&2; exit 1; }
  objects=()
  while IFS= read -r -d '' object; do objects+=("$object"); done < <(find "$build_objects" -maxdepth 1 -type f -name '*.o' -print0 | sort -z)
  ((${#objects[@]} == 7)) || { echo "expected 7 lmcad link objects; found ${#objects[@]}" >&2; exit 1; }
  relative_objects=()
  for object in "${objects[@]}"; do relative_objects+=("${object#"$BUILD_DIR"/}"); done
  archive="$DIST/$(basename "$binary")-relink-objects.tar.gz"
  tar -czf "$archive" -C "$BUILD_DIR" "${relative_objects[@]}" -C "$DIST_DIR" LICENSE THIRD_PARTY_NOTICES OCCT-RELINKING.md
elif [[ "$PLATFORM" == mingw ]]; then
  # MinGW-w64 static libstdc++/libgcc: GPL-3 with the GCC runtime exception.
  cp "$ROOT/licenses/GPL-3.txt" "$DIST/GPL-3.txt"
  cp "$ROOT/licenses/GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt" "$DIST/GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt"
  # Relinking recipe: ship the exact OCCT flags and build script instead of
  # the Linux object archive (see durable-memory/13-licensing.md).
  cp "$ROOT/.github/occt-cmake-flags.txt" "$DIST/occt-cmake-flags.txt"
  cp "$ROOT/scripts/build-occt.sh" "$DIST/build-occt.sh"
elif [[ "$PLATFORM" == darwin ]]; then
  # Only Apple system libraries (libSystem, libc++) remain, dynamically.
  cp "$ROOT/.github/occt-cmake-flags.txt" "$DIST/occt-cmake-flags.txt"
  cp "$ROOT/scripts/build-occt.sh" "$DIST/build-occt.sh"
else
  echo "unknown platform: $PLATFORM" >&2
  exit 2
fi
