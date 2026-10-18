# Build, run and test

## Toolchain and SDK

- **Checked 2026-10-18:** Linux ARM64 (`aarch64`, Neoverse-V1), 4 CPUs, 15 GiB RAM; g++ 13.3.0, CMake 3.28.3, Ninja 1.11.1.
- **Compiler:** lmscript >= 0.2.328 provides `_dsl_` / `savedsl`; this host's `test.sh` requires that minimum. The investigation box initially had 0.2.298, which was upgraded to 0.2.333 for schema and example compilation.
- **SDK:** `sdk.lock` pins cpp-dsl-sdk commit `2273dfcc133824c643ff2cda6f0fbd5387ef5c26` and the SHA-256 of `cpp/dslvm.hpp`. `scripts/sync-sdk.sh` refreshes the header from `../cpp-dsl-sdk` (override with `CPP_DSL_SDK`); `scripts/sync-sdk.sh --verify` checks the pinned header hash.
- **Dependencies:** no renderer, PNG or mesh-export library is linked. PNG uses an in-tree encoder with stored DEFLATE blocks; PPM, STL and OBJ are emitted directly.

## Build and example flow

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
lmscript examples/still-life.lms build/still-life.dslb
build/lmcad --dsl build/still-life.dslb --schema schema/lmcad.dsl.json
lmscript examples/box-stl.lms build/box-stl.dslb
build/lmcad --dsl build/box-stl.dslb --schema schema/lmcad.dsl.json
```

The first command chain writes `build/still-life.png` (640x480); the second writes `build/box.stl`, a binary STL containing 12 facets. `lmcad` accepts exactly the bytecode file and the schema file used to compile it. Loader errors return 1, CLI argument errors 2, and DSL/host runtime errors 3.

## Test results

`bash test.sh` passed **11 checks** on 2026-10-18 with lmscript 0.2.333. The checks cover the SDK pin, Release/Ninja build, both compiler examples, both host runs, PNG signature and 640x480 IHDR, PPM header and OBJ vertex/face output, 12-facet STL size, center/background and visible-pixel checks, an independent PNG CRC-32/Adler-32 parse, identical raster/PNG bytes for repeated unit inputs, schema hash refusal (exit 1), bad argument refusal (exit 2), and missing-camera validation (exit 3).

The still-life image was opened for visual review: red box, blue faceted sphere and gold cylinder are visible against a dark blue background, with direct lighting.

## Link checks

The default Release binary applies `-static-libstdc++ -static-libgcc`; on this ARM64 box it was 1,854,360 bytes and `ldd build/lmcad` showed:

```text
linux-vdso.so.1
libm.so.6 => /lib/aarch64-linux-gnu/libm.so.6
libc.so.6 => /lib/aarch64-linux-gnu/libc.so.6
/lib/ld-linux-aarch64.so.1
```

A full-static build succeeded with `-DLMCAD_FULL_STATIC=ON`. `file build-static/lmcad` reports a statically linked AArch64 ELF; `ldd build-static/lmcad` reports `not a dynamic executable`. It was 2,501,720 bytes and successfully ran both example bytecode files. These measurements apply to this box/toolchain; hosted builds should record per-OS closure and size.

## Useful commands

```sh
scripts/sync-sdk.sh --verify
bash test.sh
cmake -S . -B build-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMCAD_FULL_STATIC=ON
cmake --build build-static --target lmcad
```
