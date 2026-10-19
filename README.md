# lmcad

`lmcad` is a headless C++ host for lmscript DSL bytecode. It embeds the exact bytes of [`schema/lmcad.dsl.json`](schema/lmcad.dsl.json), vendors the pinned header-only [`cpp-dsl-sdk`](cpp/dslvm.hpp), and uses OpenCASCADE for solid modeling. OCCT shapes are tessellated into lmcad's mesh; the in-tree rasterizer, PNG/PPM writers, and STL/OBJ exporters consume that mesh. The release executable is fully statically linked and has no runtime shared-library dependencies. OCCT is licensed under LGPL-2.1 with the Open CASCADE exception; review its redistribution terms before shipping.

## Build OpenCASCADE once

OCCT 8.0.1 is a native build dependency. On this aarch64 Linux setup, the clean static geometry build takes about 16 minutes and peaks around 386 MiB RSS. The configured set disables Draw, Visualization, ApplicationFramework and DataExchange; there is no X11, OpenGL, Tcl, FreeType or TBB dependency. The prefix is reused by lmcad and fresh clones.

```sh
git clone --depth 1 --branch V8_0_1 https://github.com/Open-Cascade-SAS/OCCT.git /home/ubuntu/poc/occt-spike/occt-src
nice -n 10 cmake -S /home/ubuntu/poc/occt-spike/occt-src \
  -B /home/ubuntu/poc/occt-spike/build-geom -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_LIBRARY_TYPE=Static \
  -DBUILD_MODULE_ApplicationFramework=OFF -DBUILD_MODULE_DataExchange=OFF \
  -DBUILD_MODULE_Draw=OFF -DBUILD_MODULE_FoundationClasses=OFF \
  -DBUILD_MODULE_ModelingAlgorithms=OFF -DBUILD_MODULE_ModelingData=OFF \
  -DBUILD_MODULE_Visualization=OFF \
  '-DBUILD_ADDITIONAL_TOOLKITS=TKPrim;TKBO;TKFillet;TKOffset;TKMesh' \
  -DBUILD_DOC_Overview=OFF -DBUILD_DOC_RefMan=OFF -DUSE_TK=OFF \
  -DUSE_FREETYPE=OFF -DUSE_TBB=OFF -DUSE_XLIB=OFF -DUSE_OPENGL=OFF \
  -DBUILD_USE_PCH=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=OFF \
  -DCMAKE_INSTALL_PREFIX=/home/ubuntu/poc/occt-prefix
/usr/bin/time -v nice -n 10 cmake --build /home/ubuntu/poc/occt-spike/build-geom --parallel 3
nice -n 10 cmake --install /home/ubuntu/poc/occt-spike/build-geom
```

`TKOffset` is the one extra toolkit beyond the spike's 14-library closure; shelling needs it. The executable links the resulting 15 static OCCT archives, without OCCT's STL/STEP exchange modules. CI can cache the OCCT build tree by tag, OS, architecture, and toolchain.

## Build

Requirements: C++17 compiler, CMake 3.24+, Ninja, and (to compile examples) an lmscript compiler with DSL v5/schema v2 support. CMake generates `schema_embedded.hpp` from the schema file; changing the schema triggers regeneration.

```sh
scripts/sync-sdk.sh --verify
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLMCAD_OCCT_PREFIX=/home/ubuntu/poc/occt-prefix -DLMCAD_FULL_STATIC=ON
cmake --build build -j3
file build/lmcad
ldd build/lmcad  # prints: not a dynamic executable
stat -c '%s bytes' build/lmcad
```

The final AArch64 binary in this configuration is 26,762,776 bytes (25.5 MiB). Set `LMCAD_OCCT_PREFIX` when using a different install location.

`test.sh` prefers `lmscript` on PATH when it accepts schema v2 and compiles a while-loop probe to v5 bytecode. Otherwise it uses `scripts/lmscript-mono.sh`, a small adapter that imports the real `~/lmctl-mono/lmscript/dsl.ts` compiler directly; no compiler copy or environment variable is needed. The adapter compiles standalone `_dsl_` blocks only. Set `LMSCRIPT_BIN` to explicitly select a compiler.

## CLI

```sh
build/lmcad --schema
build/lmcad --schema --json > /tmp/lmcad-schema.json
cmp schema/lmcad.dsl.json /tmp/lmcad-schema.json
build/lmcad --dsl-signatures
scripts/lmscript-mono.sh --dsl-signatures schema/lmcad.dsl.json
scripts/lmscript-mono.sh examples/still-life.lms build/still-life.dslb
build/lmcad --dsl build/still-life.dslb
```

`--schema` prints a compact registry-derived summary with context first, then free functions and members grouped by type. `--schema --json` prints the embedded schema bytes exactly. `--dsl-signatures` prints the host registry's canonical signature list, which must match the compiler's output for the schema. `--dsl FILE [FILE ...]` executes bytecode files in argument order, using one embedded schema and the same host context for every block. Each file has fresh VM locals, while shapes, materials, camera and other host state persist across files. Execution stops on the first load or runtime error (exit 3); previous outputs and any output already written by the failing block remain. Each completed block prints one success JSON line, followed by an error JSON line if a later block fails. All command-line arguments are validated before execution (invalid syntax exits 2). The old `--schema FILE` input form is removed.

The examples cover a lit still life, STL export, a range-loop row, CSV-driven tweening, a plate with a loop of through-holes, a hollow brick with loop-generated studs, a revolved hollow vase, and a 5x5 scalar-colored glyph field. `fit_camera(fov, margin)` frames visible shapes; scene settings provide key and fill lights, ambient level, and bounded 1..4 supersampling (default 2). OCCT tessellation computes smooth normals across curved faces while keeping sharp edges. Boolean operations and solid-volume assertions use the BRep geometry. To compile and run them individually:

```sh
scripts/lmscript-mono.sh examples/still-life.lms build/still-life.dslb
scripts/lmscript-mono.sh examples/box-stl.lms build/box-stl.dslb
scripts/lmscript-mono.sh examples/loop-row.lms build/loop-row.dslb
scripts/lmscript-mono.sh examples/tween.lms build/tween.dslb
build/lmcad --dsl build/still-life.dslb
build/lmcad --dsl build/box-stl.dslb
build/lmcad --dsl build/loop-row.dslb
build/lmcad --dsl build/tween.dslb
scripts/lmscript-mono.sh examples/plate-with-holes.lms build/plate-with-holes.dslb
scripts/lmscript-mono.sh examples/brick.lms build/brick.dslb
scripts/lmscript-mono.sh examples/vase.lms build/vase.dslb
build/lmcad --dsl build/plate-with-holes.dslb
build/lmcad --dsl build/brick.dslb
build/lmcad --dsl build/vase.dslb
scripts/lmscript-mono.sh examples/glyph-field.lms build/glyph-field.dslb
build/lmcad --dsl build/glyph-field.dslb
```

## Checks

```sh
./test.sh
cmake -S . -B build-sanitize -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' \
  -DLMCAD_OCCT_PREFIX=/home/ubuntu/poc/occt-prefix -DLMCAD_FULL_STATIC=OFF
LMCAD_FULL_STATIC=OFF BUILD_DIR=build-sanitize ./test.sh
```

The test script checks SDK pinning, v5 example compilation, JSON schema byte identity, complete summary/signature consistency, sequential multi-file execution, PNG contents and determinism, bounded supersampling, 12-triangle box STL, tween output, CAD PNG/STL exports, CAD operation volume assertions, and host-side validation refusals. The SDK loader rejects every bytecode version except v5. See [`durable-memory/03-build-run-test.md`](durable-memory/03-build-run-test.md), [`durable-memory/04-gotchas.md`](durable-memory/04-gotchas.md), and [`durable-memory/05-open-items.md`](durable-memory/05-open-items.md).
