#!/usr/bin/env bash
# Regenerate every image in docs/images/ from the self-executing examples.
# Each example writes images/<name>.png (and .gif for animations) into the
# current directory; this script runs them from the repository root and
# copies the gallery set into docs/images/.
#
# lmcad: build/lmcad (or LMCAD_BIN, or lmcad on PATH). Examples that also use
# lmplot need it on PATH, in the lmscript tool cache, or GITHUB_TOKEN set so
# lmscript can install it. GIFs need ffmpeg.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$HERE"
BUILD_DIR="${BUILD_DIR:-build}"

if [ -n "${LMCAD_BIN:-}" ]; then
  export PATH="$(cd "$(dirname "$LMCAD_BIN")" && pwd):$PATH"
elif [ -x "$BUILD_DIR/lmcad" ] || [ -x "$BUILD_DIR/lmcad.exe" ]; then
  export PATH="$HERE/$BUILD_DIR:$PATH"
fi
for tool_dir in "${HOME:-}/.cache/lmscript/tools/github/lmctl-ai"/*/latest; do
  [ -d "$tool_dir" ] && export PATH="$PATH:$tool_dir"
done
command -v lmscript >/dev/null || { echo "error: lmscript not found (npm i -g @lmctl-ai/lmctl)" >&2; exit 2; }
command -v ffmpeg >/dev/null || { echo "error: ffmpeg is needed for the GIF animations" >&2; exit 2; }

mkdir -p build images docs/images
for src in examples/*.lms; do
  name="$(basename "$src" .lms)"
  echo "== $name"
  lmscript "$src" "build/$name.dslb"
done

gallery="gear-train.gif coil-spring.png
truss-bridge.png beam-sections.png pressure-vessel.png
heat-sink.png buckyball.png dipole-field.png
mug.png vase.png brick.png plate-with-holes.png
caffeine-molecule.png dna-double-helix.png cornell-box.png saturn-rings.png
pbr-spheres.png material-sandbox.png model-viewer.png primitives.png
multidraw.png cubemap.png bullet.png fluidsimulation3d.png fluidsimulation2d.png textured-triangle.png
level-1-sessions.png level-1-sessions.gif level-2-orchestration.png level-2-orchestration.gif
level-3-hands-off-lead.png level-3-hands-off-lead.gif level-4-automation.png level-4-automation.gif"
for f in $gallery; do
  cp "images/$f" "docs/images/$f"
done
mkdir -p docs/videos
cp images/level-*.mp4 docs/videos/ 2>/dev/null || true
python3 - $gallery <<'PY'
import sys, pathlib, struct
for name in sys.argv[1:]:
    p = pathlib.Path("docs/images") / name
    b = p.read_bytes()
    limit = 3_000_000 if name.endswith(".gif") else 300_000
    if len(b) > limit:
        raise SystemExit(f"{p} is {len(b)} bytes (limit {limit})")
    w, h = struct.unpack_from(">II", b, 16) if b[:8] == b"\x89PNG\r\n\x1a\n" else struct.unpack_from("<HH", b, 6)
    print(f"{p}: {w}x{h}, {len(b)} bytes")
PY
echo "docs/images/ updated"
