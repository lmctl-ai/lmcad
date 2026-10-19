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

for example in still-life box-stl loop-row tween plate-with-holes brick vase glyph-field section-view mug turntable; do
  "$LMSCRIPT" "examples/$example.lms" "build/$example.dslb" >"$WORK/$example.log" || fail "$example compile with $COMPILER_SOURCE"
  [ "$(od -An -t u1 -j 5 -N 1 "build/$example.dslb" | tr -d ' ')" = 5 ] || fail "$example was not compiled as bytecode v5"
done
ok "11 examples compile with $COMPILER_SOURCE as bytecode v5"

"$HOST" --schema --json >"$WORK/embedded-schema.json" || fail "embedded schema JSON print"
cmp schema/lmcad.dsl.json "$WORK/embedded-schema.json" || fail "embedded schema differs from source bytes"
ok "--schema --json prints byte-identical embedded schema bytes"
"$HOST" --schema >"$WORK/schema-summary.txt" || fail "schema summary print"
python3 tests/schema-cli.py "$HOST"
ok "--schema summary lists every canonical registry member"
diff -u <("$LMSCRIPT" --dsl-signatures schema/lmcad.dsl.json) <("$HOST" --dsl-signatures) || fail "host/compiler signatures differ"
ok "typed registry signatures match compiler schema signatures"

rm -f build/tween-*.png
"$HOST" --dsl build/still-life.dslb || fail "still-life run"
"$HOST" --dsl build/box-stl.dslb || fail "box STL run"
"$HOST" --dsl build/loop-row.dslb || fail "loop row run"
"$HOST" --dsl build/tween.dslb || fail "tween run"
"$HOST" --dsl build/plate-with-holes.dslb || fail "plate with holes run"
"$HOST" --dsl build/brick.dslb || fail "brick run"
"$HOST" --dsl build/vase.dslb || fail "vase run"
"$HOST" --dsl build/glyph-field.dslb || fail "glyph field run"
"$HOST" --dsl build/section-view.dslb || fail "section view run"
"$HOST" --dsl build/mug.dslb || fail "mug run"
"$HOST" --dsl build/turntable.dslb || fail "turntable run"
ok "still-life, STL, loop/tween, CAD models, section, mug, and turntable programs run"
"$HOST" --dsl build/loop-row.dslb build/loop-row.dslb >"$WORK/multi.jsonl" || fail "multi-bytecode invocation"
[ "$(wc -l < "$WORK/multi.jsonl" | tr -d ' ')" = 2 ] || fail "multiple bytecode files did not run in order"
ok "multiple bytecode files run sequentially against one embedded schema"

python3 - "build" <<'PY'
import struct,sys,zlib,hashlib,pathlib
bdir=pathlib.Path(sys.argv[1]); b=(bdir/'still-life.png').read_bytes(); assert b[:8]==b'\x89PNG\r\n\x1a\n'
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
assert (bdir/'loop-row.png').stat().st_size>1000
for name in ('plate-with-holes','brick','vase','glyph-field','section-view','mug'):
    image=(bdir/f'{name}.png').read_bytes(); assert image[:8]==b'\x89PNG\r\n\x1a\n'
    ww,hh=struct.unpack_from('>II',image,16); expected=(500,375) if name=='mug' else (560,420) if name=='vase' else (720,540); assert (ww,hh)==expected,(name,ww,hh)
    facets=(bdir/f'{name}.stl').read_bytes(); count=struct.unpack_from('<I',facets,80)[0]; assert count>(10 if name=='section-view' else 100) and len(facets)==84+count*50
for name in ('section-view','mug'):
    image=(bdir/f'{name}.png').read_bytes(); assert image[:8]==b'\x89PNG\r\n\x1a\n'
print(f'PNG/PPM/OBJ valid; CAD PNG dimensions include reduced vase/mug renders, STL box facets={n}, CAD STL facets checked, tween frames={len(frames)}, unique={len(set(hashes))}')
PY
ok "PNG dimensions/pixels, STL facets, CAD exports, loop render, glyph field, and 30 distinct tween frames"
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
for example in plate-with-holes brick vase glyph-field section-view mug; do
  cp "build/$example.png" "$WORK/$example.before.png"
  cp "build/$example.stl" "$WORK/$example.before.stl"
  "$HOST" --dsl "build/$example.dslb" >/dev/null || fail "$example deterministic rerun"
  cmp "build/$example.png" "$WORK/$example.before.png" || fail "$example PNG changed between runs"
  cmp "build/$example.stl" "$WORK/$example.before.stl" || fail "$example STL changed between runs"
done
python3 tests/cad-exports.py
ok "CAD PNG/STL determinism, closed oriented surfaces, and approximate mesh volumes"

set +e; out="$("$HOST" 2>&1)"; code=$?; set -e
[ "$code" = 2 ] && [[ "$out" == *'usage: lmcad'* ]] || fail "bad args refusal (exit $code): $out"
ok "bad arguments refused (exit 2)"
set +e; out="$("$HOST" --schema schema/lmcad.dsl.json 2>&1)"; code=$?; set -e
[ "$code" = 2 ] && [[ "$out" == *'usage: lmcad'* ]] || fail "removed schema-file option accepted (exit $code): $out"
ok "old --schema FILE input option refused (exit 2)"

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
