#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="${1:?dist directory}"
OCCT_PREFIX="${2:?OCCT install prefix}"
mkdir -p "$DIST"
cp "$ROOT/THIRD_PARTY_NOTICES.md" "$DIST/THIRD_PARTY_NOTICES"
cp "$OCCT_PREFIX/share/doc/opencascade/LICENSE_LGPL_21.txt" "$DIST/OCCT-LICENSE_LGPL_21.txt"
cp "$OCCT_PREFIX/share/doc/opencascade/OCCT_LGPL_EXCEPTION.txt" "$DIST/OCCT-LGPL-EXCEPTION.txt"
cp /usr/share/common-licenses/LGPL-2.1 "$DIST/LGPL-2.1.txt"
cp /usr/share/common-licenses/GPL-3 "$DIST/GPL-3.txt"
gcc_major="$(g++ -dumpfullversion -dumpversion | cut -d. -f1)"
cp "/usr/share/doc/gcc-${gcc_major}-base/copyright" "$DIST/GCC-TOOLCHAIN-COPYRIGHT"
cp /usr/share/doc/libc6/copyright "$DIST/GLIBC-COPYRIGHT"
