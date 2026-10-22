<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/readme/hero-dark.png">
    <source media="(prefers-color-scheme: light)" srcset="docs/readme/hero-light.png">
    <img src="docs/readme/hero-light.png" alt="A rendered aluminum mounting plate with eight clean through-holes, made by lmcad" width="100%">
  </picture>
</p>

**lmcad turns parametric lmscript into solid geometry, rendered PNGs, and STL meshes.**

[![Latest release](https://img.shields.io/github/v/release/lmctl-ai/lmcad?label=release)](https://github.com/lmctl-ai/lmcad/releases)
[![CI](https://github.com/lmctl-ai/lmcad/actions/workflows/ci.yml/badge.svg)](https://github.com/lmctl-ai/lmcad/actions/workflows/ci.yml)

## Made with lmcad

| Measured box | Repeated geometry |
| --- | --- |
| ![Rendered box](docs/readme/gallery/box-volume.png) | ![Seven-box row](docs/readme/gallery/loop-row.png) |
| One solid with a checked volume. [Source](examples/box-volume.lms) · [example book](https://lmctl.com/tools/lmcad/examples/box-volume.html) | A range loop places seven boxes. [Source](examples/loop-row.lms) · [example book](https://lmctl.com/tools/lmcad/examples/loop-row.html) |

| Lit still life | Revolved and joined solids |
| --- | --- |
| ![Lit scene with box, sphere and cylinder](docs/readme/gallery/still-life.png) | ![Rendered handled ceramic mug](docs/readme/gallery/mug.png) |
| Three primitives in one scene. [Source](examples/still-life.lms) · [example book](https://lmctl.com/tools/lmcad/examples/still-life.html) | A hollow, filleted mug with a joined handle. [Source](examples/mug.lms) · [example book](https://lmctl.com/tools/lmcad/examples/mug.html) |

## How it works

The flow below is drawn with lmdraw from [docs/readme/architecture.lms](docs/readme/architecture.lms). lmscript compiles against lmcad's declared schema; the native host uses OpenCASCADE to build solids and export images or meshes.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/readme/architecture.png">
  <img src="docs/readme/architecture.png" alt="LMScript source and the lmcad schema flow through OpenCASCADE and the mesh renderer to PNG or STL output" width="100%">
</picture>

## Quick start

```sh
npm install -g @lmctl-ai/lmctl
lmctl init
lmctl autorun https://lmctl.com/tools/lmcad/lmcad_solo.lms
```

For a local script, declare the released host with `cli="lmcad" install="github:lmctl-ai/lmcad@latest"` and follow the [get started guide](https://lmctl.com/tools/lmcad/get_started.html).

## Learn and download

- [Tool book](https://lmctl.com/tools/lmcad/) · [Get started](https://lmctl.com/tools/lmcad/get_started.html) · [API guide](docs/LLM_GUIDE.md)
- [Release downloads](https://github.com/lmctl-ai/lmcad/releases) · [Example source](examples/README.md)

Regenerate all README artwork and example previews with `docs/readme/render.sh`. Its lmscript source files live in `docs/readme/`.

## Build OpenCASCADE once

OCCT 8.0.1 is a native build dependency. On this aarch64 Linux setup, the clean static geometry build takes about 16 minutes and peaks around 386 MiB RSS. The configured set disables Draw, Visualization, ApplicationFramework and DataExchange; there is no X11, OpenGL, Tcl, FreeType or TBB dependency. The prefix is reused by lmcad and fresh clones.

```sh
OCCT_SRC="$PWD/build/occt-src" OCCT_BUILD="$PWD/build/occt-build" \
  OCCT_PREFIX="$PWD/build/occt-prefix" scripts/build-occt.sh
```

`TKOffset` is the one extra toolkit beyond the spike's 14-library closure; shelling needs it. The executable links the resulting 15 static OCCT archives, without OCCT's STL/STEP exchange modules. CI caches the installed prefix by OCCT version, the SHA-256 of `.github/occt-cmake-flags.txt`, runner architecture, and exact GCC version. The prefix is cached instead of the build tree. In Actions run 39264695761, the cold OCCT install took 34m24s on x64 and 20m58s on arm64 (three build jobs); runner performance varies. The warm run 39270104147 restored both prefixes and finished each complete Linux job in 76s (x64) / 74s (arm64), with OCCT compilation skipped.

## Build

Requirements: C++17 compiler, CMake 3.24+, Ninja, and (to compile examples) the published lmscript compiler, which requires Node.js >=24.15.0 and supports DSL v5/schema v2. CMake generates `schema_embedded.hpp` from the schema file; changing the schema triggers regeneration.

```sh
scripts/sync-sdk.sh --verify
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLMCAD_OCCT_PREFIX="$PWD/build/occt-prefix" -DLMCAD_FULL_STATIC=ON
cmake --build build -j3
file build/lmcad
ldd build/lmcad  # prints: not a dynamic executable
stat -c '%s bytes' build/lmcad
```

The verified AArch64 release binary is 26,767,080 bytes (25.5 MiB). Set `LMCAD_OCCT_PREFIX` when using a different install location.

`test.sh` selects `lmscript` on `PATH` only when it accepts this schema and compiles a while-loop probe to v5 bytecode. Install the published compiler with `npm i -g @lmctl-ai/lmctl`, or set `LMSCRIPT_BIN` to an explicit v5 compiler executable. If neither is available, selection fails with an install hint. For local compiler-source development only, set `LMSCRIPT_SOURCE` to the compiler source directory and run `scripts/lmscript-source.sh`; this adapter handles standalone `_dsl_` blocks.

## CLI

```sh
build/lmcad --dsl-schema
build/lmcad --dsl-schema --json > /tmp/lmcad-schema.json
cmp schema/lmcad.dsl.json /tmp/lmcad-schema.json
build/lmcad --dsl-signatures
lmscript --dsl-signatures schema/lmcad.dsl.json
lmscript examples/still-life.lms build/still-life.dslb
build/lmcad --dsl build/still-life.dslb
```

`--version` prints `lmcad X.Y.Z` (the CMake project version; a release tag must be `vX.Y.Z`, which CI checks, because lmscript `install="github:lmctl-ai/lmcad@latest"` compares the two). `--dsl-schema` prints a compact registry-derived summary with context first, then free functions and members grouped by type. `--dsl-schema --json` prints the embedded schema bytes exactly. `--dsl-signatures` prints the host registry's canonical signature list, which must match the compiler's output for the schema. `--dsl FILE [FILE ...]` executes bytecode files in argument order, using one embedded schema and the same host context for every block. Each file has fresh VM locals, while shapes, materials, camera and other host state persist across files. Execution stops on the first load or runtime error (exit 3); previous outputs and any output already written by the failing block remain. Each completed block prints one success JSON line, followed by an error JSON line if a later block fails. All command-line arguments are validated before execution (invalid syntax exits 2). The old `--dsl-schema FILE` input form is removed.

The examples cover a lit still life, STL export, a range-loop row, CSV-driven tweening, a plate with through-holes, hollow brick with studs, hollow vase, scalar-colored glyph field, a separated two-piece section view, a handled mug, and a camera turntable. Tween and turntable produce APNG animations as well as numbered PNG frames (tween). Tween is 320x240 with 30 frames at 30 FPS; turntable is 160x120 with 36 frames at 24 FPS. PNG and APNG use deterministic LZ77/fixed-Huffman DEFLATE (stored blocks for incompressible data). APNG uses exact rational frame delays (integer FPS from 1 through 240) and loops forever; `lmcad-apng-extract` extracts a zero-based full frame to PNG. Host limits are 240 frames, 64 million total frame pixels, and 16,777,216 pixels per frame. Frames stream to a temporary file in the destination directory; memory does not grow with frame count. `finish()` atomically replaces the destination. Omitting it discards the temporary animation when the host exits; an empty finish, second finish, or add after finish is an error. Write failures preserve the previous destination and remove temporary output. Frame-count and cumulative-pixel checks precede rendering; the renderer also checks the supersampled dimensions and pixel count before allocating. A 36-frame 1920x1080 animation exceeds the cumulative limit (74,649,600 pixels); at that resolution at most 30 frames fit, with supersampling at most 2. `fit_camera(fov, margin)` snapshots visible bounds and adapts the fitted distance to each render’s aspect ratio; FOV is vertical and margin must be at least 1. Explicit camera `look_at` or `clipping` calls disable automatic fitting. Ambient must be finite and nonnegative; values above 1 clamp to full ambient illumination. Scene settings provide key and fill lights, ambient level, and bounded 1..8 supersampling (default 2). OCCT tessellation uses analytic surface normals, blends them across nearly tangent face seams, and keeps sharper B-rep creases crisp. The default mesh uses 0.05 linear deflection and 0.1-radian angular deflection. Materials support bounded Blinn-Phong highlights through `specular(strength)` and `shininess(power)`; key and fill directions are tuned for stronger face contrast. Boolean operations and solid-volume assertions use the BRep geometry. To compile and run them individually:

The tween example writes a numbered frame sequence and APNG. Regenerate the documented middle still frame (`build/tween-15.png`) from the repository root with:

```sh
lmscript examples/tween.lms build/tween.dslb
build/lmcad --dsl build/tween.dslb
```

The output is 320x240, with the shared CAD example background and 2x supersampling for antialiased sphere edges.

```sh
lmscript examples/still-life.lms build/still-life.dslb
lmscript examples/box-stl.lms build/box-stl.dslb
lmscript examples/loop-row.lms build/loop-row.dslb
lmscript examples/tween.lms build/tween.dslb
build/lmcad --dsl build/still-life.dslb
build/lmcad --dsl build/box-stl.dslb
build/lmcad --dsl build/loop-row.dslb
build/lmcad --dsl build/tween.dslb
lmscript examples/plate-with-holes.lms build/plate-with-holes.dslb
lmscript examples/brick.lms build/brick.dslb
lmscript examples/vase.lms build/vase.dslb
build/lmcad --dsl build/plate-with-holes.dslb
build/lmcad --dsl build/brick.dslb
build/lmcad --dsl build/vase.dslb
lmscript examples/glyph-field.lms build/glyph-field.dslb
build/lmcad --dsl build/glyph-field.dslb
lmscript examples/section-view.lms build/section-view.dslb
lmscript examples/mug.lms build/mug.dslb
lmscript examples/turntable.lms build/turntable.dslb
build/lmcad --dsl build/section-view.dslb
build/lmcad --dsl build/mug.dslb
build/lmcad --dsl build/turntable.dslb
```

## Test

```sh
./test.sh
cmake -S . -B build-sanitize -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' \
  -DLMCAD_OCCT_PREFIX="$PWD/build/occt-prefix" -DLMCAD_FULL_STATIC=OFF
LMCAD_FULL_STATIC=OFF BUILD_DIR=build-sanitize ./test.sh
```

The test script checks SDK pinning, v5 example compilation, JSON schema byte identity, complete summary/signature consistency, sequential multi-file execution, PNG contents and determinism, bounded supersampling, STL topology/volume, CAD exports and assertions, APNG chunk order/CRCs/sequences/delays/frame counts, extracted-frame identity, changed frames, and host-side validation refusals. The SDK loader rejects every bytecode version except v5. See [`durable-memory/03-build-run-test.md`](durable-memory/03-build-run-test.md), [`durable-memory/04-gotchas.md`](durable-memory/04-gotchas.md), and [`durable-memory/05-open-items.md`](durable-memory/05-open-items.md).

Linux CI builds static OCCT once per runner architecture/toolchain cache key, then builds the fully static host and runs `test.sh`. The verified cold run produced 30,116,488-byte x64 and 26,767,080-byte arm64 executables; warm jobs passed all 26 CI checks and `ldd` reported `not a dynamic executable`. The prefix caches were 27.47 MiB (x64) and 27.77 MiB (arm64). On `v*` tags it attaches `lmcad-linux-x64`, `lmcad-linux-arm64`, the license/notice files, and an OCCT relinking-recipe archive. macOS (arm64 via macos-15, x64 via macos-15-intel) and Windows (MinGW-w64) jobs are enabled: macOS links OCCT statically with AppleClang and asserts `otool -L` shows only system libraries; Windows produces a standalone `lmcad.exe` (only system DLLs). See [`durable-memory/13-licensing.md`](durable-memory/13-licensing.md) and [`durable-memory/14-ci-macos-windows.md`](durable-memory/14-ci-macos-windows.md) before redistributing binaries.

Supersampling uses an n×n box filter over all subpixels, decoding sRGB bytes to linear light for averaging and encoding the result back to sRGB. Output dimensions stay unchanged, including odd sizes. Both raster dimensions after multiplication by n must be at most 8192, with at most 16,777,216 high-resolution pixels; this bounds the RGB/depth buffers before allocation. The renderer uses a diffuse plus Blinn-Phong display-colour model; it is not physically based.

## License

lmcad-authored code is licensed under the MIT License; see [`LICENSE`](LICENSE).
This also covers the vendored `cpp/dslvm.hpp`. OCCT and copied Python reference
examples retain their separate licenses and notices listed in
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md). Every Linux release artifact
includes `LICENSE`, `THIRD_PARTY_NOTICES`, the relevant full license texts, and
OCCT relinking instructions and object files. The npm package layouts declare MIT and `private: true`; the workflow does not
publish them.

Made by **Lattice Systems LLC**.
