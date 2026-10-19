#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"
LMSCRIPT="${LMSCRIPT_BIN:-$HERE/scripts/lmscript-mono.sh}"
BUILD_DIR="${BUILD_DIR:-build}"
HOST="$BUILD_DIR/lmcad"
UNIT="$BUILD_DIR/lmcad-unit"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/lmcad-test-XXXXXX")"
pass=0
ok(){ pass=$((pass+1)); echo "ok $pass - $1"; }
fail(){ echo "FAIL: $*" >&2; exit 1; }
"$LMSCRIPT" --version | grep -q 'DSL v4' || fail "v4 lmscript compiler unavailable"
scripts/sync-sdk.sh --verify >/dev/null || fail "SDK header pin mismatch"
ok "SDK header matches sdk.lock"
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j4
ok "host and unit test build"
for example in still-life box-stl loop-row tween; do "$LMSCRIPT" "examples/$example.lms" "build/$example.dslb" >"$WORK/$example.log"; done
ok "4 examples compile with the in-tree lmscript v4 compiler"
diff -u <("$LMSCRIPT" --dsl-signatures schema/lmcad.dsl.json) <("$HOST" --dsl-signatures schema/lmcad.dsl.json) || fail "host/schema signatures differ"
ok "typed host registry signatures match compiler schema signatures"
rm -f build/tween-*.png
"$HOST" --dsl "build/still-life.dslb" --schema schema/lmcad.dsl.json >"$WORK/still.json" || fail "still-life run"
"$HOST" --dsl "build/box-stl.dslb" --schema schema/lmcad.dsl.json >"$WORK/stl.json" || fail "box STL run"
"$HOST" --dsl "build/loop-row.dslb" --schema schema/lmcad.dsl.json >"$WORK/row.json" || fail "loop row run"
"$HOST" --dsl "build/tween.dslb" --schema schema/lmcad.dsl.json >"$WORK/tween.json" || fail "tween run"
ok "still-life, STL, range-loop, and tween programs run"
python3 - "build" <<'PY'
import struct,sys,zlib,hashlib,pathlib
bdir=pathlib.Path(sys.argv[1]); b=(bdir/'still-life.png').read_bytes(); assert b[:8]==b'\x89PNG\r\n\x1a\n'
w,h=struct.unpack_from('>II',b,16); assert (w,h)==(640,480)
s=(bdir/'box.stl').read_bytes(); n=struct.unpack_from('<I',s,80)[0]; assert n==12 and len(s)==84+50*n
assert (bdir/'still-life.ppm').read_bytes().startswith(b'P6\n64 48\n255\n')
obj=(bdir/'still-life.obj').read_text(); assert '\nv ' in obj and '\nf ' in obj
raw=b''.join(data for typ,data in []); chunks=[]; pos=8
while pos<len(b):
 nbytes=struct.unpack_from('>I',b,pos)[0]; typ=b[pos+4:pos+8]; data=b[pos+8:pos+8+nbytes]; assert zlib.crc32(typ+data)&0xffffffff==struct.unpack_from('>I',b,pos+8+nbytes)[0]; chunks.append((typ,data)); pos+=nbytes+12
raw=zlib.decompress(b''.join(d for t,d in chunks if t==b'IDAT')); stride=w*3+1
center=raw[(h//2)*stride+1+3*(w//2):(h//2)*stride+4+3*(w//2)]; assert center != bytes((20,26,41)),center
frames=sorted(bdir.glob('tween-*.png')); assert len(frames)==30,len(frames)
hashes=[hashlib.sha256(p.read_bytes()).digest() for p in frames]; assert len(set(hashes))==30
assert (bdir/'loop-row.png').stat().st_size>1000
print(f'PNG 640x480, PPM/OBJ records valid, STL triangles=12, loop PNG bytes={(bdir/"loop-row.png").stat().st_size}, tween frames={len(frames)}, unique={len(set(hashes))}')
PY
ok "PNG dimensions/pixels, STL facets, loop render, and 30 distinct tween frames"
"$UNIT" "$WORK" || fail "C++ unit checks"
ok "PNG CRC/Adler and raster determinism checks"
# Re-run the animation, byte-for-byte check all numbered images.
python3 - "build" "$WORK/tween-before.sha256" <<'PY'
import hashlib,pathlib,sys
p=pathlib.Path(sys.argv[1]); pathlib.Path(sys.argv[2]).write_text('\n'.join(hashlib.sha256(x.read_bytes()).hexdigest() for x in sorted(p.glob('tween-*.png'))))
PY
"$HOST" --dsl "build/tween.dslb" --schema schema/lmcad.dsl.json >"$WORK/tween2.json" || fail "tween deterministic rerun"
python3 - "build" "$WORK/tween-before.sha256" <<'PY'
import hashlib,pathlib,sys
p=pathlib.Path(sys.argv[1]); files=sorted(p.glob('tween-*.png')); assert len(files)==30; hashes='\n'.join(hashlib.sha256(x.read_bytes()).hexdigest() for x in files); assert hashes==pathlib.Path(sys.argv[2]).read_text(); print('30 deterministic frame files present')
PY
ok "tween frame rerun completes deterministically"
cp schema/lmcad.dsl.json "$WORK/other-schema.json"; echo ' ' >> "$WORK/other-schema.json"
set +e; out="$("$HOST" --dsl "build/still-life.dslb" --schema "$WORK/other-schema.json" 2>&1)"; code=$?; set -e
[ "$code" = 3 ] && [[ "$out" == *'schema mismatch'* ]] || fail "wrong schema refusal: exit $code: $out"
ok "wrong schema bytes refused (exit 3)"
set +e; out="$("$HOST" 2>&1)"; code=$?; set -e
[ "$code" = 2 ] && [[ "$out" == *'--dsl FILE and --schema FILE are required'* ]] || fail "bad args refusal (exit $code): $out"
ok "bad arguments refused (exit 2)"
cat > "$WORK/no-camera.lms" <<EOF2
p = _dsl_ schema="$HERE/schema/lmcad.dsl.json";
scene.render_png("$WORK/no-camera.png", 8, 8);
end _dsl_
savedsl(p, "$WORK/no-camera.dslb");
EOF2
"$LMSCRIPT" "$WORK/no-camera.lms" >/dev/null
set +e; out="$("$HOST" --dsl "$WORK/no-camera.dslb" --schema schema/lmcad.dsl.json 2>&1)"; code=$?; set -e
[ "$code" = 3 ] && [[ "$out" == *'render requires a camera'* ]] || fail "camera validation refusal (exit $code): $out"
ok "camera required validation (exit 3)"
LMSCRIPT_BIN="$LMSCRIPT" LMCAD_HOST="$HOST" python3 tests/refusals.py "$WORK"
ok "compiled host validation refusals"
LMSCRIPT_BIN="$LMSCRIPT" LMCAD_HOST="$HOST" python3 tests/tween.py "$WORK"
ok "multi-segment tween boundaries and endpoints"
python3 tests/adapter.py "$WORK"
ok "relocated compiler adapter parsing"
echo "PASS: $pass checks"
