#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root"
compiler="$(scripts/select-compiler.sh)"
lmcad_bin="${LMCAD_BIN:-$(command -v lmcad || true)}"
cache_root="${XDG_CACHE_HOME:-$HOME/.cache}/lmscript/tools/github/lmctl-ai"
lmdraw_bin="${LMDRAW_BIN:-$cache_root/lmdraw/latest/lmdraw}"
[[ -x "$lmcad_bin" ]] || { echo "error: build/install lmcad and set LMCAD_BIN" >&2; exit 2; }
[[ -x "$lmdraw_bin" ]] || { echo "error: install lmdraw@latest through lmscript or set LMDRAW_BIN" >&2; exit 2; }
mkdir -p build/rendered docs/readme/gallery examples/renders

# The checked-in sources declare their install= release. A cached compiler
# cannot resolve @latest when GitHub's release API is rate limited, so compile
# temporary copies against the exact schema bytes reported by those hosts.
"$lmcad_bin" --dsl-schema --json > build/readme-lmcad.dsl.json
"$lmdraw_bin" --dsl-schema --json > build/readme-lmdraw.dsl.json
sed 's|_dsl_ cli="lmcad" install="github:lmctl-ai/lmcad@latest";|_dsl_ schema="../../schema/lmcad.dsl.json";|' docs/readme/hero-dark.lms > docs/readme/.hero-dark-render.lms
sed 's|_dsl_ cli="lmcad" install="github:lmctl-ai/lmcad@latest";|_dsl_ schema="../../schema/lmcad.dsl.json";|' docs/readme/hero-light.lms > docs/readme/.hero-light-render.lms
sed 's|_dsl_ cli="lmdraw" install="github:lmctl-ai/lmdraw@latest";|_dsl_ schema="../../build/readme-lmdraw.dsl.json";|' docs/readme/architecture.lms > docs/readme/.architecture-render.lms
trap 'rm -f docs/readme/.hero-dark-render.lms docs/readme/.hero-light-render.lms docs/readme/.architecture-render.lms' EXIT
for name in hero-dark hero-light; do
  "$compiler" "docs/readme/.$name-render.lms" "build/readme-$name.dslb"
  "$lmcad_bin" --dsl "build/readme-$name.dslb" >/dev/null
  cp "build/rendered/readme-$name.png" "docs/readme/$name.png"
done
"$compiler" docs/readme/.architecture-render.lms build/readme-architecture.dslb
"$lmdraw_bin" --dsl build/readme-architecture.dslb >/dev/null
cp build/rendered/readme-architecture.png docs/readme/architecture.png

# Render a compact gallery directly from the repository's actual example files.
for name in box-volume loop-row still-life mug; do
  "$compiler" "examples/$name.lms" "build/readme-$name.dslb"
  "$lmcad_bin" --dsl "build/$name.dslb" >/dev/null
  cp "build/$name.png" "docs/readme/gallery/$name.png"
done
cp docs/readme/gallery/still-life.png examples/renders/still-life.png
cp docs/readme/gallery/mug.png examples/renders/mug.png
python3 - <<'PY'
from pathlib import Path
images = [Path("docs/readme") / f"{name}.png" for name in ("hero-dark", "hero-light", "architecture")]
images += [Path("docs/readme/gallery") / f"{name}.png" for name in ("box-volume", "loop-row", "still-life", "mug")]
for image in images:
    if not image.is_file() or image.stat().st_size == 0:
        raise SystemExit(f"missing generated README image: {image}")
    if image.stat().st_size > 400_000:
        raise SystemExit(f"README image exceeds 400 KB: {image} ({image.stat().st_size} bytes)")
print(f"README images ready: {len(images)} files, each under 400 KB")
PY
