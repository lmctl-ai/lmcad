#!/usr/bin/env bash
# Lay out private, unpublished platform npm packages from release binaries.
# Usage: scripts/npm-layout.sh <artifacts-dir> <out-dir> <semver>
set -euo pipefail
ART="${1:?artifacts dir}"
OUT="${2:?output dir}"
VER="${3:-0.0.0}"
[[ "$VER" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$ ]] || { echo "version '$VER' is not semver (x.y.z)" >&2; exit 2; }
PLATFORMS="linux-x64 linux-arm64"
RELEASE_NOTICES=(LICENSE THIRD_PARTY_NOTICES Apache-2.0.txt build123d-NOTICE.txt PyVista-MIT.txt GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt OCCT-LICENSE_LGPL_21.txt OCCT-LGPL-EXCEPTION.txt LGPL-2.1.txt GPL-3.txt GCC-TOOLCHAIN-COPYRIGHT GLIBC-COPYRIGHT OCCT-RELINKING.md)
for notice in "${RELEASE_NOTICES[@]}"; do
  [[ -f "$ART/$notice" ]] || { echo "missing release notice: $ART/$notice" >&2; exit 1; }
done
rm -rf "$OUT"
mkdir -p "$OUT/lmcad/bin" "$OUT/@lmcad"
cat > "$OUT/lmcad/package.json" <<EOF
{
  "name": "lmcad",
  "version": "$VER",
  "description": "Static native CAD CLI for lmscript DSL bytecode",
  "license": "MIT",
  "private": true,
  "files": ["bin", "LICENSE", "THIRD_PARTY_NOTICES", "Apache-2.0.txt", "build123d-NOTICE.txt", "PyVista-MIT.txt", "GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt", "OCCT-LICENSE_LGPL_21.txt", "OCCT-LGPL-EXCEPTION.txt", "LGPL-2.1.txt", "GPL-3.txt", "GCC-TOOLCHAIN-COPYRIGHT", "GLIBC-COPYRIGHT", "OCCT-RELINKING.md"],
  "bin": { "lmcad": "bin/lmcad.js" },
  "optionalDependencies": {
    "@lmcad/linux-x64": "$VER",
    "@lmcad/linux-arm64": "$VER"
  }
}
EOF
cat > "$OUT/lmcad/bin/lmcad.js" <<'EOF'
#!/usr/bin/env node
const { execFileSync } = require('node:child_process');
const path = require('node:path');
const key = `${process.platform}-${process.arch}`;
if (!['linux-x64', 'linux-arm64'].includes(key)) {
  console.error(`lmcad: unsupported platform ${process.platform}-${process.arch}`);
  process.exit(1);
}
const pkg = `@lmcad/${key}`;
const bin = path.join(path.dirname(require.resolve(`${pkg}/package.json`)), 'lmcad');
try { execFileSync(bin, process.argv.slice(2), { stdio: 'inherit' }); }
catch (error) { process.exit(typeof error.status === 'number' ? error.status : 1); }
EOF
chmod +x "$OUT/lmcad/bin/lmcad.js"
for plat in $PLATFORMS; do
  src="$ART/lmcad-$plat"
  [[ -f "$src" ]] || { echo "missing artifact: $src" >&2; exit 1; }
  [[ -f "$ART/lmcad-$plat-relink-objects.tar.gz" ]] || { echo "missing relink objects for $plat" >&2; exit 1; }
  dir="$OUT/@lmcad/$plat"
  mkdir -p "$dir"
  cp "$src" "$dir/lmcad"
  chmod +x "$dir/lmcad"
  cat > "$dir/package.json" <<EOF
{
  "name": "@lmcad/$plat",
  "version": "$VER",
  "description": "lmcad binary for $plat",
  "license": "MIT",
  "private": true,
  "os": ["linux"],
  "cpu": ["${plat##*-}"],
  "files": ["lmcad", "lmcad-${plat}-relink-objects.tar.gz", "LICENSE", "THIRD_PARTY_NOTICES", "Apache-2.0.txt", "build123d-NOTICE.txt", "PyVista-MIT.txt", "GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt", "OCCT-LICENSE_LGPL_21.txt", "OCCT-LGPL-EXCEPTION.txt", "LGPL-2.1.txt", "GPL-3.txt", "GCC-TOOLCHAIN-COPYRIGHT", "GLIBC-COPYRIGHT", "OCCT-RELINKING.md"]
}
EOF
  for notice in "${RELEASE_NOTICES[@]}" "lmcad-$plat-relink-objects.tar.gz"; do
    cp "$ART/$notice" "$dir/$notice"
  done
done
for notice in "${RELEASE_NOTICES[@]}"; do
  cp "$ART/$notice" "$OUT/lmcad/$notice"
done
echo "private npm layout created under $OUT (not published)"
