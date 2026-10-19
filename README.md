# lmcad

`lmcad` is a headless C++ host for lmscript DSL bytecode. It embeds the exact bytes of [`schema/lmcad.dsl.json`](schema/lmcad.dsl.json), vendors the pinned header-only [`cpp-dsl-sdk`](cpp/dslvm.hpp), and uses OpenCASCADE for solid modeling. OCCT shapes are tessellated into lmcad's mesh; the in-tree rasterizer, PNG/PPM writers, and STL/OBJ exporters consume that mesh. The release executable is fully statically linked and has no runtime shared-library dependencies. OCCT is licensed under LGPL-2.1 with the Open CASCADE exception; review its redistribution terms before shipping.

## Build OpenCASCADE once

OCCT 8.0.1 is a native build dependency. On this aarch64 Linux setup, the clean static geometry build takes about 16 minutes and peaks around 386 MiB RSS. The configured set disables Draw, Visualization, ApplicationFramework and DataExchange; there is no X11, OpenGL, Tcl, FreeType or TBB dependency. The prefix is reused by lmcad and fresh clones.

```sh
OCCT_SRC="$PWD/build/occt-src" OCCT_BUILD="$PWD/build/occt-build" \
  OCCT_PREFIX="$PWD/build/occt-prefix" scripts/build-occt.sh
```

`TKOffset` is the one extra toolkit beyond the spike's 14-library closure; shelling needs it. The executable links the resulting 15 static OCCT archives, without OCCT's STL/STEP exchange modules. CI caches the installed prefix by OCCT version, the SHA-256 of `.github/occt-cmake-flags.txt`, runner architecture, and exact GCC version. The prefix is cached instead of the build tree. In Actions run 39264695761, the cold OCCT install took 34m24s on x64 and 20m58s on arm64 (three build jobs); runner performance varies. The warm run 39270104147 restored both prefixes and finished each complete Linux job in 76s (x64) / 74s (arm64), with OCCT compilation skipped.

## Build

Requirements: C++17 compiler, CMake 3.24+, Ninja, and (to compile examples) an lmscript compiler with DSL v5/schema v2 support. CMake generates `schema_embedded.hpp` from the schema file; changing the schema triggers regeneration.

```sh
scripts/sync-sdk.sh --verify
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLMCAD_OCCT_PREFIX="$PWD/build/occt-prefix" -DLMCAD_FULL_STATIC=ON
cmake --build build -j3
file build/lmcad
ldd build/lmcad  # prints: not a dynamic executable
stat -c '%s bytes' build/lmcad
```

The final AArch64 binary in this configuration is 26,762,776 bytes (25.5 MiB). Set `LMCAD_OCCT_PREFIX` when using a different install location.

`test.sh` prefers `lmscript` on PATH when it accepts schema v2 and compiles a while-loop probe to v5 bytecode. CI installs the published npm `@lmctl-ai/lmctl@0.2.347` compiler and uses this probe; otherwise it uses `scripts/lmscript-mono.sh`, a small adapter that imports the compiler sources from a local lmscript checkout (`LMSCRIPT_SOURCE`; maintainers only, the sources are not public). Strangers: `npm i -g @lmctl-ai/lmctl`. The adapter compiles standalone `_dsl_` blocks only. Set `LMSCRIPT_BIN` to explicitly select a compiler.

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

The examples cover a lit still life, STL export, a range-loop row, CSV-driven tweening, a plate with through-holes, hollow brick with studs, hollow vase, scalar-colored glyph field, a separated two-piece section view, a handled mug, and a camera turntable. Tween and turntable produce APNG animations as well as numbered PNG frames (tween). Both animations are 160x120; tween has 30 frames at 30 FPS and turntable has 36 frames at 24 FPS. PNG and APNG use deterministic LZ77/fixed-Huffman DEFLATE (stored blocks for incompressible data). APNG uses exact rational frame delays (integer FPS from 1 through 240) and loops forever; `lmcad-apng-extract` extracts a zero-based full frame to PNG. Host limits are 240 frames, 64 million total frame pixels, and 16,777,216 pixels per frame. Frames stream to a temporary file in the destination directory; memory does not grow with frame count. `finish()` atomically replaces the destination. Omitting it discards the temporary animation when the host exits; an empty finish, second finish, or add after finish is an error. Write failures preserve the previous destination and remove temporary output. Frame-count and cumulative-pixel checks precede rendering; the renderer also checks the supersampled dimensions and pixel count before allocating. A 36-frame 1920x1080 animation exceeds the cumulative limit (74,649,600 pixels); at that resolution at most 30 frames fit, with supersampling at most 2. `fit_camera(fov, margin)` snapshots visible bounds and adapts the fitted distance to each render’s aspect ratio; FOV is vertical and margin must be at least 1. Explicit camera `look_at` or `clipping` calls disable automatic fitting. Ambient must be finite and nonnegative; values above 1 clamp to full ambient illumination. Scene settings provide key and fill lights, ambient level, and bounded 1..4 supersampling (default 2). OCCT tessellation computes smooth normals across curved faces while keeping sharp edges. Boolean operations and solid-volume assertions use the BRep geometry. To compile and run them individually:

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
scripts/lmscript-mono.sh examples/section-view.lms build/section-view.dslb
scripts/lmscript-mono.sh examples/mug.lms build/mug.dslb
scripts/lmscript-mono.sh examples/turntable.lms build/turntable.dslb
build/lmcad --dsl build/section-view.dslb
build/lmcad --dsl build/mug.dslb
build/lmcad --dsl build/turntable.dslb
```

## Checks

```sh
./test.sh
cmake -S . -B build-sanitize -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' \
  -DLMCAD_OCCT_PREFIX="$PWD/build/occt-prefix" -DLMCAD_FULL_STATIC=OFF
LMCAD_FULL_STATIC=OFF BUILD_DIR=build-sanitize ./test.sh
```

The test script checks SDK pinning, v5 example compilation, JSON schema byte identity, complete summary/signature consistency, sequential multi-file execution, PNG contents and determinism, bounded supersampling, STL topology/volume, CAD exports and assertions, APNG chunk order/CRCs/sequences/delays/frame counts, extracted-frame identity, changed frames, and host-side validation refusals. The SDK loader rejects every bytecode version except v5. See [`durable-memory/03-build-run-test.md`](durable-memory/03-build-run-test.md), [`durable-memory/04-gotchas.md`](durable-memory/04-gotchas.md), and [`durable-memory/05-open-items.md`](durable-memory/05-open-items.md).

Linux CI builds static OCCT once per runner architecture/toolchain cache key, then builds the fully static host and runs `test.sh`. The verified cold run produced 30,116,488-byte x64 and 26,767,080-byte arm64 executables; warm jobs passed all 26 CI checks and `ldd` reported `not a dynamic executable`. The prefix caches were 27.47 MiB (x64) and 27.77 MiB (arm64). On `v*` tags it attaches `lmcad-linux-x64`, `lmcad-linux-arm64`, the license/notice files, and an OCCT relinking-recipe archive. `.github/workflows/ci.yml` includes manual-only macOS and Windows placeholders; macOS needs platform-specific non-`-static` link settings, while Windows needs a MinGW-w64 route and a POSIX API port. See [`durable-memory/13-licensing.md`](durable-memory/13-licensing.md) before redistributing binaries.

Supersampling uses an n×n box filter over all subpixels, decoding sRGB bytes to linear light for averaging and encoding the result back to sRGB. Output dimensions stay unchanged, including odd sizes. Both raster dimensions after multiplication by n must be at most 8192, with at most 16,777,216 high-resolution pixels; this bounds the RGB/depth buffers before allocation. The Lambert shader remains a simple display-colour model, not a physically based renderer.
