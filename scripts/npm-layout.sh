#!/usr/bin/env bash
# Lay out private, unpublished platform npm packages from release binaries.
# Usage: scripts/npm-layout.sh <artifacts-dir> <out-dir> <semver>
set -euo pipefail
ART="${1:?artifacts dir}"
OUT="${2:?output dir}"
VER="${3:-0.0.0}"
[[ "$VER" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$ ]] || { echo "version '$VER' is not semver (x.y.z)" >&2; exit 2; }
PLATFORMS="linux-x64 linux-arm64 macos-x64 macos-arm64 windows-x64"
COMMON_NOTICES=(LICENSE THIRD_PARTY_NOTICES Apache-2.0.txt build123d-NOTICE.txt PyVista-MIT.txt OCCT-LICENSE_LGPL_21.txt OCCT-LGPL-EXCEPTION.txt LGPL-2.1.txt)
LINUX_EXTRA=(GPL-3.txt GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt GCC-TOOLCHAIN-COPYRIGHT GLIBC-COPYRIGHT OCCT-RELINKING.md)
MINGW_EXTRA=(GPL-3.txt GCC-RUNTIME-LIBRARY-EXCEPTION-3.1.txt occt-cmake-flags.txt build-occt.sh)
DARWIN_EXTRA=(occt-cmake-flags.txt build-occt.sh)

npm_os() { case "$1" in macos*) echo darwin;; windows*) echo win32;; *) echo "${1%%-*}";; esac; }
npm_cpu() { echo "${1##*-}"; }
npm_bin() { [[ "$1" == windows-* ]] && echo lmcad.exe || echo lmcad; }
notices_for() {
  case "$1" in
    linux-*)   echo "${COMMON_NOTICES[*]} ${LINUX_EXTRA[*]}";;
    macos-*)   echo "${COMMON_NOTICES[*]} ${DARWIN_EXTRA[*]}";;
    windows-*) echo "${COMMON_NOTICES[*]} ${MINGW_EXTRA[*]}";;
  esac
}

# The merged artifact dir carries the union of every platform's notices.
ALL_NOTICES=($(printf '%s\n' "${COMMON_NOTICES[@]}" "${LINUX_EXTRA[@]}" "${MINGW_EXTRA[@]}" | sort -u))
for notice in "${ALL_NOTICES[@]}"; do
  [[ -f "$ART/$notice" ]] || { echo "missing release notice: $ART/$notice" >&2; exit 1; }
done
for plat in $PLATFORMS; do
  [[ -f "$ART/lmcad-$plat" || -f "$ART/lmcad-$plat.exe" ]] || { echo "missing artifact: $ART/lmcad-$plat" >&2; exit 1; }
  if [[ "$plat" == linux-* && ! -f "$ART/lmcad-$plat-relink-objects.tar.gz" ]]; then
    echo "missing relink objects for $plat" >&2; exit 1
  fi
  for notice in $(notices_for "$plat"); do
    [[ -f "$ART/$notice" ]] || { echo "missing notice $notice for $plat" >&2; exit 1; }
  done
done

json_list() { local sep="" out=""; for f in "$@"; do out="$out$sep\"$f\""; sep=", "; done; printf '%s' "$out"; }

rm -rf "$OUT"
mkdir -p "$OUT/lmcad/bin" "$OUT/@lmcad"
MAIN_FILES=("bin" "${ALL_NOTICES[@]}")
{
  printf '{\n  "name": "lmcad",\n  "version": "%s",\n' "$VER"
  printf '  "description": "Static native CAD CLI for lmscript DSL bytecode",\n'
  printf '  "license": "MIT",\n  "private": true,\n'
  printf '  "files": [%s],\n' "$(json_list "${MAIN_FILES[@]}")"
  printf '  "bin": { "lmcad": "bin/lmcad.js" },\n  "optionalDependencies": {\n'
  i=0
  for p in $PLATFORMS; do
    [ $i -gt 0 ] && printf ',\n'
    printf '    "@lmcad/%s": "%s"' "$p" "$VER"
    i=$((i+1))
  done
  printf '\n  }\n}\n'
} > "$OUT/lmcad/package.json"
cat > "$OUT/lmcad/bin/lmcad.js" <<'EOF'
#!/usr/bin/env node
const { execFileSync } = require('node:child_process');
const path = require('node:path');
const map = {
  'linux-x64': '@lmcad/linux-x64',
  'linux-arm64': '@lmcad/linux-arm64',
  'darwin-x64': '@lmcad/macos-x64',
  'darwin-arm64': '@lmcad/macos-arm64',
  'win32-x64': '@lmcad/windows-x64',
};
const key = `${process.platform}-${process.arch}`;
const pkg = map[key];
if (!pkg) {
  console.error(`lmcad: unsupported platform ${key}`);
  process.exit(1);
}
const binName = process.platform === 'win32' ? 'lmcad.exe' : 'lmcad';
const bin = path.join(path.dirname(require.resolve(`${pkg}/package.json`)), binName);
try { execFileSync(bin, process.argv.slice(2), { stdio: 'inherit' }); }
catch (error) { process.exit(typeof error.status === 'number' ? error.status : 1); }
EOF
chmod +x "$OUT/lmcad/bin/lmcad.js"
for plat in $PLATFORMS; do
  src="$ART/lmcad-$plat"
  bin="$(npm_bin "$plat")"
  [ -f "$src.exe" ] && src="$src.exe"
  dir="$OUT/@lmcad/$plat"
  mkdir -p "$dir"
  cp "$src" "$dir/$bin"
  chmod +x "$dir/$bin"
  read -ra notices <<< "$(notices_for "$plat")"
  files=("$bin" "${notices[@]}")
  if [[ "$plat" == linux-* ]]; then files+=("lmcad-$plat-relink-objects.tar.gz"); fi
  {
    printf '{\n  "name": "@lmcad/%s",\n  "version": "%s",\n' "$plat" "$VER"
    printf '  "description": "lmcad binary for %s",\n' "$plat"
    printf '  "license": "MIT",\n  "private": true,\n'
    printf '  "os": ["%s"],\n  "cpu": ["%s"],\n' "$(npm_os "$plat")" "$(npm_cpu "$plat")"
    printf '  "files": [%s]\n}\n' "$(json_list "${files[@]}")"
  } > "$dir/package.json"
  for notice in "${notices[@]}"; do cp "$ART/$notice" "$dir/$notice"; done
  if [[ "$plat" == linux-* ]]; then cp "$ART/lmcad-$plat-relink-objects.tar.gz" "$dir/"; fi
done
for notice in "${ALL_NOTICES[@]}"; do cp "$ART/$notice" "$OUT/lmcad/$notice"; done
echo "private npm layout created under $OUT (not published)"
