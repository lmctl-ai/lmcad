# lmcad

`lmcad` is a native, headless C++ host for lmscript `_dsl_` bytecode. A DSL chunk builds a scene through the schema in `schema/lmcad.dsl.json`; the host embeds the pinned `cpp-dsl-sdk` VM, owns the scene objects, then writes still images or triangle mesh exports. It currently renders boxes, spheres and cylinders with a perspective camera and directional light.

## Requirements

- C++17 compiler, CMake 3.24+, Ninja.
- `lmscript` 0.2.328 or newer for `_dsl_` / `savedsl` (tests were run with 0.2.333).
- Python 3 is used by the end-to-end checks to inspect PNG and STL output. The renderer itself has no third-party runtime library.

## Build and run

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
lmscript examples/still-life.lms build/still-life.dslb
build/lmcad --dsl build/still-life.dslb --schema schema/lmcad.dsl.json
lmscript examples/box-stl.lms build/box-stl.dslb
build/lmcad --dsl build/box-stl.dslb --schema schema/lmcad.dsl.json
```

The still-life render is written to `build/still-life.png`, with auxiliary PPM and OBJ outputs; the box example writes binary STL to `build/box.stl`. The host accepts a bytecode path and the exact schema bytes used at compile time:

```text
lmcad --dsl FILE.dslb --schema SCHEMA.dsl.json
```

## Fully static build

The normal build statically links libstdc++ and libgcc. On this Linux ARM64 target it leaves only libc and libm as dynamic libraries. A fully static build also works here:

```sh
cmake -S . -B build-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMCAD_FULL_STATIC=ON
cmake --build build-static --target lmcad
```

The 2026-10-18 full-static ARM64 binary was 2,501,720 bytes. Target-OS builds should still be measured independently.

## Test

```sh
bash test.sh
```

The script verifies the SDK pin, compiles both examples, runs the host, inspects PNG signature/dimensions and visible pixels, checks PPM/OBJ output and that the binary STL has 12 box triangles, runs PNG CRC/Adler and raster determinism checks, and checks schema mismatch, argument and missing-camera refusals.
