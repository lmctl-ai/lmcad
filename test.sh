#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"
BUILD_DIR="${BUILD_DIR:-build}"
HOST="$BUILD_DIR/lmcad"
UNIT="$BUILD_DIR/lmcad-unit"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/lmcad-test-XXXXXX")"
pass=0
ok(){ pass=$((pass+1)); echo "ok $pass - $1"; }
fail(){ echo "FAIL: $*" >&2; exit 1; }

if [ -n "${LMSCRIPT_BIN:-}" ]; then
  LMSCRIPT="$LMSCRIPT_BIN"
  COMPILER_SOURCE="explicit LMSCRIPT_BIN"
elif command -v lmscript >/dev/null 2>&1 && lmscript --dsl-signatures schema/lmcad.dsl.json >"$WORK/path-signatures" 2>/dev/null; then
  LMSCRIPT="$(command -v lmscript)"
  COMPILER_SOURCE="PATH lmscript"
else
  LMSCRIPT="$HERE/scripts/lmscript-mono.sh"
  COMPILER_SOURCE="mono dsl.ts adapter"
fi
scripts/sync-sdk.sh --verify >/dev/null || fail "SDK header pin mismatch"
ok "SDK header matches sdk.lock"
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j4
ok "host and unit test build"

for example in still-life box-stl loop-row tween; do
  "$LMSCRIPT" "examples/$example.lms" "build/$example.dslb" >"$WORK/$example.log" || fail "$example compile with $COMPILER_SOURCE"
  [ "$(od -An -t u1 -j 5 -N 1 "build/$example.dslb" | tr -d ' ')" = 5 ] || fail "$example was not compiled as bytecode v5"
done
ok "4 examples compile with $COMPILER_SOURCE as bytecode v5"

"$HOST" --schema >"$WORK/embedded-schema.json" || fail "embedded schema print"
cmp schema/lmcad.dsl.json "$WORK/embedded-schema.json" || fail "embedded schema differs from source bytes"
ok "--schema prints byte-identical embedded schema bytes"
diff -u <("$LMSCRIPT" --dsl-signatures schema/lmcad.dsl.json) <("$HOST" --dsl-signatures) || fail "host/compiler signatures differ"
ok "typed registry signatures match compiler schema signatures"

rm -f build/tween-*.png
"$HOST" --dsl build/still-life.dslb || fail "still-life run"
"$HOST" --dsl build/box-stl.dslb || fail "box STL run"
"$HOST" --dsl build/loop-row.dslb || fail "loop row run"
"$HOST" --dsl build/tween.dslb || fail "tween run"
ok "still-life, STL, range-loop, and tween programs run"
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
print(f'PNG 640x480, PPM/OBJ valid, STL triangles={n}, tween frames={len(frames)}, unique={len(set(hashes))}')
PY
ok "PNG dimensions/pixels, STL facets, loop render, and 30 distinct tween frames"
"$UNIT" "$WORK" || fail "C++ unit checks"
ok "PNG CRC/Adler and raster determinism checks"

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

set +e; out="$("$HOST" 2>&1)"; code=$?; set -e
[ "$code" = 2 ] && [[ "$out" == *'usage: lmcad'* ]] || fail "bad args refusal (exit $code): $out"
ok "bad arguments refused (exit 2)"
set +e; out="$("$HOST" --schema schema/lmcad.dsl.json 2>&1)"; code=$?; set -e
[ "$code" = 2 ] && [[ "$out" == *'usage: lmcad'* ]] || fail "removed schema-file option accepted (exit $code): $out"
ok "old --schema FILE input option refused (exit 2)"

cat > "$WORK/no-camera.lms" <<EOF2
p = _dsl_ schema="$HERE/schema/lmcad.dsl.json";
scene.render_png("$WORK/no-camera.png", 8, 8);
end _dsl_
savedsl(p, "$WORK/no-camera.dslb");
EOF2
"$LMSCRIPT" "$WORK/no-camera.lms" >/dev/null || fail "no-camera DSL compile"
set +e; out="$("$HOST" --dsl "$WORK/no-camera.dslb" 2>&1)"; code=$?; set -e
[ "$code" = 3 ] && [[ "$out" == *'render requires a camera'* ]] || fail "camera validation refusal (exit $code): $out"
ok "camera required validation (exit 3)"
LMSCRIPT_BIN="$LMSCRIPT" LMCAD_HOST="$HOST" python3 tests/refusals.py "$WORK"
ok "compiled host validation refusals"
LMSCRIPT_BIN="$LMSCRIPT" LMCAD_HOST="$HOST" python3 tests/tween.py "$WORK"
ok "multi-segment tween boundaries and endpoints"
python3 tests/adapter.py "$WORK"
ok "relocated compiler adapter parsing"
echo "PASS: $pass checks"
