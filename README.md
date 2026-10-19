# lmcad

`lmcad` is a native, headless C++ host for lmscript DSL bytecode. Scripts build scenes through [`schema/lmcad.dsl.json`](schema/lmcad.dsl.json); the host embeds the pinned `cpp-dsl-sdk` VM and writes PNG/PPM images and STL/OBJ meshes. The current schema includes range-loop scenes and CSV-driven keyframe animation rendered as numbered PNGs.

## Requirements

- C++17 compiler, CMake 3.24+, Ninja.
- Node.js with TypeScript stripping (`node --experimental-strip-types`) and a `~/lmctl-mono/lmscript` checkout for v4 DSL compilation. `LMSCRIPT_SOURCE` can point `scripts/lmscript-mono.mjs` at another checkout.
- Python 3 for the integration checks. The native renderer has no third-party runtime library.

The released `lmscript` on PATH is still v3 and cannot compile the v4 examples. `scripts/lmscript-mono.sh` is a thin adapter around the in-tree `dsl.ts` compiler; it does not install packages or modify that checkout. It supports exactly one standalone DSL block, not general lmscript execution. Supply an output argument unless `savedsl` uses a literal path for that block. The default compiler location is relative to the current user’s home, independent of where lmcad is cloned; set `LMSCRIPT_SOURCE` when the compiler lives elsewhere.

## Build and run

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
scripts/lmscript-mono.sh examples/still-life.lms build/still-life.dslb
build/lmcad --dsl build/still-life.dslb --schema schema/lmcad.dsl.json
scripts/lmscript-mono.sh examples/box-stl.lms build/box-stl.dslb
build/lmcad --dsl build/box-stl.dslb --schema schema/lmcad.dsl.json
scripts/lmscript-mono.sh examples/loop-row.lms build/loop-row.dslb
build/lmcad --dsl build/loop-row.dslb --schema schema/lmcad.dsl.json
scripts/lmscript-mono.sh examples/tween.lms build/tween.dslb
build/lmcad --dsl build/tween.dslb --schema schema/lmcad.dsl.json
```

The examples write `build/still-life.png`, `build/box.stl`, `build/loop-row.png`, and 30 files named `build/tween-0.png` through `build/tween-29.png`. The tween reads `examples/data/keyframes.csv`; DSL code uses `range`, `lerp`, and `ease`. To compare the host's declared API with the compiler's schema view, run:

```sh
scripts/lmscript-mono.sh --dsl-signatures schema/lmcad.dsl.json > /tmp/schema-signatures
build/lmcad --dsl-signatures schema/lmcad.dsl.json > /tmp/host-signatures
diff -u /tmp/schema-signatures /tmp/host-signatures
```

## Static linking

The normal Release build uses `-static-libstdc++ -static-libgcc`. A full static build is available with `-DLMCAD_FULL_STATIC=ON`:

```sh
cmake -S . -B build-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMCAD_FULL_STATIC=ON
cmake --build build-static --target lmcad
```

## Tests

```sh
./test.sh
cmake -S . -B build-sanitize -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
BUILD_DIR=build-sanitize ./test.sh
```
