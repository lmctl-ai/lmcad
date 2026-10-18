#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"
LMSCRIPT="${LMSCRIPT_BIN:-lmscript}"
MIN_LMSCRIPT=0.2.328
have="$($LMSCRIPT --version 2>/dev/null | sed -n 's/^lmscript //p')"
if [ -z "$have" ] || [ "$(printf '%s\n%s\n' "$MIN_LMSCRIPT" "$have" | sort -V | head -1)" != "$MIN_LMSCRIPT" ]; then
  echo "FAIL: lmscript ${have:-unknown}; _dsl_ needs >= $MIN_LMSCRIPT" >&2; exit 1
fi
pass=0
ok(){ pass=$((pass+1)); echo "ok $pass - $1"; }
fail(){ echo "FAIL: $*" >&2; exit 1; }
scripts/sync-sdk.sh --verify >/dev/null || fail "SDK header pin mismatch"
ok "SDK header matches sdk.lock"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ok "Release host and unit test build"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/lmcad-test-XXXXXX")"
"$LMSCRIPT" examples/still-life.lms build/still-life.dslb >"$WORK/compile-image.log"
"$LMSCRIPT" examples/box-stl.lms build/box-stl.dslb >"$WORK/compile-stl.log"
ok "both examples compile with lmscript $have"
build/lmcad --dsl build/still-life.dslb --schema schema/lmcad.dsl.json || fail "still-life host run"
ok "still-life DSL runs and writes PNG"
build/lmcad --dsl build/box-stl.dslb --schema schema/lmcad.dsl.json || fail "box STL host run"
ok "box STL DSL runs and writes binary STL"
python3 - <<'PY'
import struct
b=open('build/still-life.png','rb').read()
assert b[:8] == b'\x89PNG\r\n\x1a\n', 'PNG signature'
w,h=struct.unpack_from('>II',b,16)
assert (w,h)==(640,480), f'PNG dimensions {w}x{h}'
print('PNG signature and IHDR 640x480 verified')
s=open('build/still-life.ppm','rb').read()
assert s.startswith(b'P6\n64 48\n255\n'), 'PPM header'
obj=open('build/still-life.obj').read()
assert obj.count('\nv ') >= 3 and '\nf ' in obj, 'OBJ vertices/faces'
print('PPM header and OBJ vertex/face records verified')
s=open('build/box.stl','rb').read()
assert len(s)>=84, 'short STL'
n=struct.unpack_from('<I',s,80)[0]
assert n==12 and len(s)==84+50*n, f'STL count/size {n}/{len(s)}'
print('binary STL triangle count 12 verified')
PY
ok "PNG signature/IHDR and STL facet count verified"
python3 - <<'PY'
import struct,zlib
b=open('build/still-life.png','rb').read(); pos=8; chunks=[]
while pos<len(b):
 n=struct.unpack_from('>I',b,pos)[0]; typ=b[pos+4:pos+8]; data=b[pos+8:pos+8+n]; chunks.append((typ,data)); pos+=12+n
raw=zlib.decompress(b''.join(d for t,d in chunks if t==b'IDAT'))
w,h=struct.unpack_from('>II',b,16); stride=w*3+1
background=(round(.08*255),round(.10*255),round(.16*255)); changed=0
for y in range(h):
 row=raw[y*stride:(y+1)*stride]; assert row[0]==0, 'unexpected PNG filter'
 changed+=sum(1 for x in range(w) if tuple(row[1+3*x:4+3*x])!=background)
center=raw[(h//2)*stride+1+3*(w//2):(h//2)*stride+1+3*(w//2)+3]
assert tuple(center)!=background, f'center pixel is background: {tuple(center)}'
assert changed>10000, f'too few non-background pixels: {changed}'
print(f'center pixel {tuple(center)}; non-background pixels {changed}')
PY
ok "render contains visible geometry, including at center pixel"
build/lmcad-unit "$WORK" || fail "C++ PNG/raster unit checks"
ok "C++ PNG CRC/Adler and raster determinism checks"
cp schema/lmcad.dsl.json "$WORK/other-schema.json"
echo ' ' >> "$WORK/other-schema.json"
set +e
out="$(build/lmcad --dsl build/still-life.dslb --schema "$WORK/other-schema.json" 2>&1)"; code=$?
set -e
[ "$code" = 1 ] && [[ "$out" == *"schema mismatch"* ]] || fail "wrong schema was not rejected (exit $code): $out"
ok "schema-hash mismatch refused before execution (exit 1)"
set +e
out="$(build/lmcad 2>&1)"; code=$?
set -e
[ "$code" = 2 ] && [[ "$out" == *"--dsl FILE and --schema FILE are required"* ]] || fail "bad arguments accepted (exit $code): $out"
ok "bad arguments refused (exit 2)"
cat > "$WORK/no-camera.lms" <<EOF
p = _dsl_ schema="$HERE/schema/lmcad.dsl.json";
let mat = material();
scene.box(1.0, 1.0, 1.0, mat);
scene.render_png("$WORK/no-camera.png", 8, 8);
end _dsl_
savedsl(p, "$WORK/no-camera.dslb");
EOF
"$LMSCRIPT" "$WORK/no-camera.lms" >/dev/null
set +e
out="$(build/lmcad --dsl "$WORK/no-camera.dslb" --schema schema/lmcad.dsl.json 2>&1)"; code=$?
set -e
[ "$code" = 3 ] && [[ "$out" == *"render requires a camera"* ]] || fail "missing camera not refused (exit $code): $out"
ok "host validation requires a camera (exit 3)"
echo "PASS: $pass checks"
