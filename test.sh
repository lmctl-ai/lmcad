#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"
BUILD_DIR="${BUILD_DIR:-build}"
UNAME_S="$(uname -s)"
EXE=""
case "$UNAME_S" in MINGW*|MSYS*|CYGWIN*) EXE=".exe";; esac
HOST="$BUILD_DIR/lmcad$EXE"
UNIT="$BUILD_DIR/lmcad-unit$EXE"
APNG="$BUILD_DIR/lmcad-apng-extract$EXE"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/lmcad-test-XXXXXX")"
# Native Windows hosts cannot address git-bash /tmp paths; the compiler also
# embeds the schema path into chunks, so both become drive-letter paths.
SCHEMA_PATH="$HERE/schema/lmcad.dsl.json"
case "$UNAME_S" in MINGW*|MSYS*|CYGWIN*)
  WORK="$(cd "$WORK" && pwd -W | tr '\\' '/')"
  SCHEMA_PATH="$(cd "$HERE" && pwd -W | tr '\\' '/')/schema/lmcad.dsl.json";;
esac
pass=0
ok(){ pass=$((pass+1)); echo "ok $pass - $1"; }
fail(){ echo "FAIL: $*" >&2; exit 1; }

LMSCRIPT="$(scripts/select-compiler.sh)"
COMPILER_SOURCE="$LMSCRIPT"
scripts/sync-sdk.sh --verify >/dev/null || fail "SDK header pin mismatch"
ok "SDK header matches sdk.lock"
# Default: scripts/build-occt.sh's prefix, else a sibling ../occt-prefix.
DEFAULT_OCCT_PREFIX="$HERE/build/occt-prefix"
[ -d "$DEFAULT_OCCT_PREFIX/include/opencascade" ] || DEFAULT_OCCT_PREFIX="$HERE/../occt-prefix"
OCCT_PREFIX="${LMCAD_OCCT_PREFIX:-$DEFAULT_OCCT_PREFIX}"
FULL_STATIC="${LMCAD_FULL_STATIC:-ON}"
# OCCT's Windows install layout has cmake/ + inc/ instead of lib/cmake/opencascade + include/opencascade.
[ -d "$OCCT_PREFIX/include/opencascade" ] || [ -f "$OCCT_PREFIX/cmake/OpenCASCADEConfig.cmake" ] || fail "OCCT prefix not found: $OCCT_PREFIX (set LMCAD_OCCT_PREFIX)"
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMCAD_OCCT_PREFIX="$OCCT_PREFIX" -DLMCAD_FULL_STATIC="$FULL_STATIC"
cmake --build "$BUILD_DIR" -j3
ok "host and unit test build"

want_version="lmcad $(sed -n 's/^project(lmcad VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
[ "$("$HOST" --version)" = "$want_version" ] || fail "--version prints $want_version"
set +e; "$HOST" --version extra >/dev/null 2>&1; code=$?; set -e
[ "$code" = 2 ] || fail "--version with an extra argument exits 2 (got $code)"
ok "--version prints exactly '$want_version' (lmscript @latest reads it)"
"$HOST" --dsl-schema --json >"$WORK/embedded-schema.json" || fail "embedded schema JSON print"
cmp schema/lmcad.dsl.json "$WORK/embedded-schema.json" || fail "embedded schema differs from source bytes"
ok "--dsl-schema --json prints byte-identical embedded schema bytes"
"$HOST" --dsl-schema >"$WORK/schema-summary.txt" || fail "schema summary print"
python3 tests/schema-cli.py "$HOST"
ok "--dsl-schema summary lists every canonical registry member"
diff -u <("$LMSCRIPT" --dsl-signatures schema/lmcad.dsl.json) <("$HOST" --dsl-signatures) || fail "host/compiler signatures differ"
ok "typed registry signatures match compiler schema signatures"

# Every example is self-executing: `lmscript examples/x.lms` compiles its _dsl_
# block(s) against the host found on PATH (cli="lmcad" -> the build under test),
# runs the bytecode with that host and writes images/<name>.png (STL/APNG/frames
# go to build/). Examples that also bind lmplot need it on PATH, in the lmscript
# tool cache, or a GITHUB_TOKEN so lmscript can install it; otherwise they are
# skipped with a message. GIF conversion needs ffmpeg (the examples report a skip).
export PATH="$HERE/$BUILD_DIR:$PATH"
if [ -d "${HOME:-}/.cache/lmscript/tools/github/lmctl-ai" ]; then
  for tool_dir in "$HOME/.cache/lmscript/tools/github/lmctl-ai"/lmplot/latest; do
    [ -d "$tool_dir" ] && export PATH="$PATH:$tool_dir"
  done
fi
have_lmplot=0
if command -v lmplot >/dev/null 2>&1 || [ -n "${GITHUB_TOKEN:-}" ]; then have_lmplot=1; fi
[ -n "$EXE" ] && have_lmplot=0
rm -rf images build/tween-*.png
mkdir -p images
ran=0; skipped=""
for src in examples/*.lms; do
  name="$(basename "$src" .lms)"
  if grep -q 'cli="lmplot"' "$src" && [ "$have_lmplot" = 0 ]; then
    skipped="$skipped $name"; echo "skip - $name needs lmplot (not on PATH, no GITHUB_TOKEN to install it)"; continue
  fi
  "$LMSCRIPT" "$src" "build/$name.dslb" >"$WORK/$name.log" 2>&1 || { cat "$WORK/$name.log" >&2; fail "$name self-executing run"; }
  [ "$(od -An -t u1 -j 5 -N 1 "build/$name.dslb" | tr -d ' ')" = 5 ] || fail "$name was not compiled as bytecode v5"
  ran=$((ran+1))
done
ok "$ran examples compile as bytecode v5 and self-execute against the built host${skipped:+ (skipped:$skipped)}"
for name in ball-stick box-volume loop-row still-life section-view mug brick vase plate-with-holes gear-train pressure-vessel coil-spring buckyball dipole-field; do
  [ -f "images/$name.png" ] || fail "images/$name.png was not written"
done
ok "self-executing examples wrote their images/*.png"
readme_lms=0
while IFS= read -r url; do
  rel="${url#https://raw.githubusercontent.com/lmctl-ai/lmcad/main/}"
  [ -f "$rel" ] || fail "README autorun target missing: $rel"
  readme_lms=$((readme_lms+1))
done < <(grep -oE 'lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/[A-Za-z0-9_.-]+\.lms' README.md | sed 's/^lmctl autorun //' | sort -u)
[ "$readme_lms" -ge 12 ] || fail "expected >= 12 README autorun examples, got $readme_lms"
grep -qiE 'see source|\[Source\]' README.md && fail "README still links to source instead of autorun"
readme_img=0
while IFS= read -r img; do
  [ -f "$img" ] || fail "README image missing: $img"
  sig="$(head -c 8 "$img" | od -An -tx1 | tr -d ' \n')"
  case "$sig" in 89504e470d0a1a0a|4749463839*|4749463837*) ;; *) fail "README image is not PNG/GIF: $img";; esac
  readme_img=$((readme_img+1))
done < <(grep -oE 'docs/images/[A-Za-z0-9_.-]+\.(png|gif)' README.md | sort -u)
[ "$readme_img" -ge 12 ] || fail "expected >= 12 README gallery images, got $readme_img"
ok "README: $readme_lms autorun examples exist, $readme_img gallery images are valid PNG/GIF"
"$HOST" --dsl build/loop-row.dslb build/loop-row.dslb >"$WORK/multi.jsonl" || fail "multi-bytecode invocation"
[ "$(wc -l < "$WORK/multi.jsonl" | tr -d ' ')" = 2 ] || fail "multiple bytecode files did not run in order"
ok "multiple bytecode files run sequentially against one embedded schema"

python3 - "build" <<'PY'
import struct,sys,zlib,hashlib,pathlib
bdir=pathlib.Path(sys.argv[1]); idir=pathlib.Path('images'); b=(idir/'still-life.png').read_bytes(); assert b[:8]==b'\x89PNG\r\n\x1a\n'
w,h=struct.unpack_from('>II',b,16); assert (w,h)==(640,480)
s=(bdir/'box.stl').read_bytes(); n=struct.unpack_from('<I',s,80)[0]; assert n==12 and len(s)==84+50*n
assert (bdir/'still-life.ppm').read_bytes().startswith(b'P6\n64 48\n255\n')
obj=(bdir/'still-life.obj').read_text(); assert '\nv ' in obj and '\nf ' in obj
chunks=[]; pos=8
while pos<len(b):
 nbytes=struct.unpack_from('>I',b,pos)[0]; typ=b[pos+4:pos+8]; data=b[pos+8:pos+8+nbytes]; assert zlib.crc32(typ+data)&0xffffffff==struct.unpack_from('>I',b,pos+8+nbytes)[0]; chunks.append((typ,data)); pos+=nbytes+12
raw=zlib.decompress(b''.join(d for t,d in chunks if t==b'IDAT')); stride=w*3+1
center=raw[(h//2)*stride+1+3*(w//2):(h//2)*stride+4+3*(w//2)]; assert center != bytes((20,26,41)),center
frames=sorted(bdir.glob('tween-*.png')); assert len(frames)==30,len(frames)
hashes=[hashlib.sha256(p.read_bytes()).digest() for p in frames]; assert len(set(hashes))==30
assert (idir/'loop-row.png').stat().st_size>1000
ball_stick=(idir/'ball-stick.png').read_bytes(); assert ball_stick[:8]==b'\x89PNG\r\n\x1a\n'; assert struct.unpack_from('>II',ball_stick,16)==(640,480)
box_volume=(idir/'box-volume.png').read_bytes(); assert box_volume[:8]==b'\x89PNG\r\n\x1a\n'; assert struct.unpack_from('>II',box_volume,16)==(320,240)
for name in ('plate-with-holes','brick','vase','section-view','mug'):
    image=(idir/f'{name}.png').read_bytes(); assert image[:8]==b'\x89PNG\r\n\x1a\n'
    ww,hh=struct.unpack_from('>II',image,16); expected=(720,540) if name=='section-view' else (900,600); assert (ww,hh)==expected,(name,ww,hh)
    facets=(bdir/f'{name}.stl').read_bytes(); count=struct.unpack_from('<I',facets,80)[0]; assert count>(10 if name=='section-view' else 100) and len(facets)==84+count*50
print(f'PNG/PPM/OBJ valid; box volume PNG=320x240; CAD PNG dimensions checked, STL box facets={n}, CAD STL facets checked, tween frames={len(frames)}, unique={len(set(hashes))}')
PY
ok "PNG dimensions/pixels, volume assertion, STL facets, CAD exports, loop render, and 30 distinct tween frames"
"$APNG" build/tween.apng 0 "$WORK/tween-apng-first.png" || fail "extract tween APNG frame 0"
cmp build/tween-0.png "$WORK/tween-apng-first.png" || fail "APNG first frame differs from PNG render"
"$APNG" build/tween.apng 15 "$WORK/tween-apng-middle.png" || fail "extract tween APNG middle frame"
"$APNG" build/tween.apng 29 "$WORK/tween-apng-last.png" || fail "extract tween APNG last frame"
cmp -s "$WORK/tween-apng-first.png" "$WORK/tween-apng-middle.png" && fail "tween APNG middle frame did not change"
cmp -s "$WORK/tween-apng-middle.png" "$WORK/tween-apng-last.png" && fail "tween APNG last frame did not change"
for frame in 0 18 35; do "$APNG" build/turntable.apng "$frame" "$WORK/turntable-$frame.png" || fail "extract turntable frame $frame"; done
cmp -s "$WORK/turntable-0.png" "$WORK/turntable-18.png" && fail "turntable middle frame did not change"
cmp -s "$WORK/turntable-18.png" "$WORK/turntable-35.png" && fail "turntable last frame did not change"
ok "APNG extracted frames decode to PNG; tween and turntable frames differ"
node tests/apng-extract.cjs "$APNG" "$WORK"
ok "APNG extraction rejects malformed structure and invalid indices"
"$UNIT" "$WORK" build || fail "C++ unit checks"
ok "PNG CRC/Adler, APNG chunk CRC/sequence/frame count, and raster determinism checks"

python3 - "build" "$WORK/tween-before.sha256" <<'PY'
import hashlib,pathlib,sys
p=pathlib.Path(sys.argv[1]); pathlib.Path(sys.argv[2]).write_text('\n'.join(hashlib.sha256(x.read_bytes()).hexdigest() for x in sorted(p.glob('tween-*.png'))))
PY
"$HOST" --dsl build/tween.dslb >/dev/null || fail "tween deterministic rerun"
python3 - "build" "$WORK/tween-before.sha256" <<'PY'
import hashlib,pathlib,sys
p=pathlib.Path(sys.argv[1]); files=sorted(p.glob('tween-*.png')); assert len(files)==30; hashes='\n'.join(hashlib.sha256(x.read_bytes()).hexdigest() for x in files); assert hashes==pathlib.Path(sys.argv[2]).read_text(); print('30 deterministic frame files present')
PY
ok "tween frame rerun completes deterministically"
cp build/tween.apng "$WORK/tween.before.apng"
cp build/turntable.apng "$WORK/turntable.before.apng"
"$HOST" --dsl build/tween.dslb >/dev/null || fail "tween APNG deterministic rerun"
cmp build/tween.apng "$WORK/tween.before.apng" || fail "tween APNG changed between runs"
"$HOST" --dsl build/turntable.dslb >/dev/null || fail "turntable APNG deterministic rerun"
cmp build/turntable.apng "$WORK/turntable.before.apng" || fail "turntable APNG changed between runs"
ok "tween and turntable APNG reruns are byte deterministic"
for example in plate-with-holes brick vase section-view mug; do
  cp "images/$example.png" "$WORK/$example.before.png"
  cp "build/$example.stl" "$WORK/$example.before.stl"
  "$HOST" --dsl "build/$example.dslb" >/dev/null || fail "$example deterministic rerun"
  cmp "images/$example.png" "$WORK/$example.before.png" || fail "$example PNG changed between runs"
  cmp "build/$example.stl" "$WORK/$example.before.stl" || fail "$example STL changed between runs"
done
python3 tests/cad-exports.py
ok "CAD PNG/STL determinism, closed oriented surfaces, and approximate mesh volumes"

set +e; out="$("$HOST" 2>&1)"; code=$?; set -e
[ "$code" = 2 ] && [[ "$out" == *'usage: lmcad'* ]] || fail "bad args refusal (exit $code): $out"
ok "bad arguments refused (exit 2)"
set +e; out="$("$HOST" --dsl-schema schema/lmcad.dsl.json 2>&1)"; code=$?; set -e
[ "$code" = 2 ] && [[ "$out" == *'usage: lmcad'* ]] || fail "removed schema-file option accepted (exit $code): $out"
ok "--dsl-schema FILE form refused (exit 2)"

cat > "$WORK/no-camera.lms" <<EOF2
p = _dsl_ schema="$SCHEMA_PATH";
scene.render_png("$WORK/no-camera.png", 8, 8);
end _dsl_
savedsl(p, "$WORK/no-camera.dslb");
EOF2
"$LMSCRIPT" "$WORK/no-camera.lms" >/dev/null || fail "no-camera DSL compile"
set +e; out="$("$HOST" --dsl "$WORK/no-camera.dslb" 2>&1)"; code=$?; set -e
[ "$code" = 3 ] && [[ "$out" == *'render requires a camera'* ]] || fail "camera validation refusal (exit $code): $out"
ok "camera required validation (exit 3)"
cat > "$WORK/bad-supersample.lms" <<EOF2
p = _dsl_ schema="$SCHEMA_PATH";
scene.supersample(9);
end _dsl_
savedsl(p, "$WORK/bad-supersample.dslb");
EOF2
"$LMSCRIPT" "$WORK/bad-supersample.lms" >/dev/null || fail "invalid supersample DSL compile"
set +e; out="$($HOST --dsl "$WORK/bad-supersample.dslb" 2>&1)"; code=$?; set -e
[ "$code" = 3 ] && [[ "$out" == *'supersample must be between 1 and 8'* ]] || fail "invalid supersample was accepted (exit $code): $out"
ok "supersampling factor is bounded to 1..8"
cat > "$WORK/bad-fillet.lms" <<EOF2
p = _dsl_ schema="$SCHEMA_PATH";
let shape = scene.box(10.0, 10.0, 10.0, material());
shape.fillet(100.0);
end _dsl_
savedsl(p, "$WORK/bad-fillet.dslb");
EOF2
"$LMSCRIPT" "$WORK/bad-fillet.lms" >/dev/null || fail "invalid fillet DSL compile"
# KNOWN LIMITATION (windows-x64, MinGW): OCCT 8.0.1's fillet/chamfer/hollow
# *error* paths segfault (exit 139) instead of failing cleanly; valid
# operations all pass. The graceful-failure contract is asserted on the
# other platforms.
if [ -z "$EXE" ]; then
  set +e; out="$("$HOST" --dsl "$WORK/bad-fillet.dslb" 2>&1)"; code=$?; set -e
  [ "$code" = 3 ] && [[ "$out" == *'fillet failed'* ]] || fail "invalid fillet did not return a host error (exit $code): $out"
  ok "invalid OCCT fillet returns a DSL host error"
else
  echo "skip - OCCT/MinGW error-path crash (known limitation, see durable-memory)"
fi
LMSCRIPT_BIN="$LMSCRIPT" LMCAD_HOST="$HOST" python3 tests/render-settings.py "$WORK"
ok "renderer settings and light handles"
LMSCRIPT_BIN="$LMSCRIPT" LMCAD_HOST="$HOST" python3 tests/cad.py "$WORK"
ok "CAD sketch validation"
LMSCRIPT_BIN="$LMSCRIPT" LMCAD_HOST="$HOST" python3 tests/refusals.py "$WORK"
ok "compiled host validation refusals"
LMSCRIPT_BIN="$LMSCRIPT" LMCAD_HOST="$HOST" python3 tests/tween.py "$WORK"
ok "multi-segment tween boundaries and endpoints"
LMSCRIPT_BIN="$LMSCRIPT" LMCAD_HOST="$HOST" python3 tests/multi-file.py "$WORK"
ok "shared scene, stop-on-failure, and upfront argument validation"
if [ -z "$EXE" ]; then
  LMCAD_REAL_COMPILER="$LMSCRIPT" python3 tests/compiler-selection.py "$WORK"
  ok "compiler selection verifies v5 output capability"
else
  echo "skip - compiler-selection probes a shell-script compiler (native Windows Python cannot exec it)"
fi
# The optional source-adapter check runs only when a compiler source directory is explicitly selected.
if [ -n "${LMSCRIPT_SOURCE:-}" ] && [ -f "$LMSCRIPT_SOURCE/dsl.ts" ]; then
  python3 tests/adapter.py "$WORK"
  ok "relocated compiler adapter parsing"
else
  echo "skip - source adapter check (set LMSCRIPT_SOURCE to a compiler source directory)"
fi

# npm layout smoke: fake binaries, real notice files.
# GNU tar reads "D:" as a remote host; BSD tar rejects --force-local.
TAR_LOCAL=""
[ -n "$EXE" ] && TAR_LOCAL="--force-local"
NPMART="$WORK/npm-art"; mkdir -p "$NPMART"
cp LICENSE "$NPMART/"; cp THIRD_PARTY_NOTICES.md "$NPMART/THIRD_PARTY_NOTICES"
cp licenses/*.txt "$NPMART/"; cp licenses/LINUX-RELINKING.md "$NPMART/OCCT-RELINKING.md"
cp .github/occt-cmake-flags.txt "$NPMART/"; cp scripts/build-occt.sh "$NPMART/"
printf 'x' > "$NPMART/OCCT-LICENSE_LGPL_21.txt"; printf 'x' > "$NPMART/OCCT-LGPL-EXCEPTION.txt"
printf 'x' > "$NPMART/GCC-TOOLCHAIN-COPYRIGHT"; printf 'x' > "$NPMART/GLIBC-COPYRIGHT"
for p in linux-x64 linux-arm64 macos-x64 macos-arm64 windows-x64; do printf 'x' > "$NPMART/lmcad-$p"; done
for p in linux-x64 linux-arm64; do tar ${TAR_LOCAL:+$TAR_LOCAL} -czf "$NPMART/lmcad-$p-relink-objects.tar.gz" -C "$NPMART" LICENSE; done
scripts/npm-layout.sh "$NPMART" "$WORK/npm-out" 1.2.3 >/dev/null || fail "npm layout"
python3 - "$WORK/npm-out" <<'PY'
import json,os,sys
out=sys.argv[1]
main=json.load(open(f'{out}/lmcad/package.json'))
assert main['license']=='MIT' and main['private'] is True, main
assert set(main['optionalDependencies'])=={f'@lmcad/{p}' for p in ('linux-x64','linux-arm64','macos-x64','macos-arm64','windows-x64')}, main
expect={'linux-x64':('linux','x64','lmcad'),'linux-arm64':('linux','arm64','lmcad'),
        'macos-x64':('darwin','x64','lmcad'),'macos-arm64':('darwin','arm64','lmcad'),
        'windows-x64':('win32','x64','lmcad.exe')}
for p,(os_,cpu,binname) in expect.items():
    j=json.load(open(f'{out}/@lmcad/{p}/package.json'))
    assert (j['os'],j['cpu'],j['license'],j['private'])==([os_],[cpu],'MIT',True), j
    assert binname in j['files'] and os.path.isfile(f'{out}/@lmcad/{p}/{binname}'), p
    assert 'LICENSE' in j['files'] and os.path.isfile(f'{out}/@lmcad/{p}/LICENSE'), p
print('npm layout: 5 platforms, MIT, private, os/cpu/files correct')
PY
ok "npm layout: five platform packages with per-platform notices"
echo "PASS: $pass checks"
