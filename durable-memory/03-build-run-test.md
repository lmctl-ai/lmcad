# Build, run and test

## Toolchain and SDK

- Checked on Linux ARM64, g++ 13.3.0, CMake 3.28.3, Ninja 1.11.1.
- DSL v4 examples use the source compiler at `/home/ubuntu/lmctl-mono/lmscript/dsl.ts`. The global released `lmscript` is v3 and cannot compile these examples. `scripts/lmscript-mono.sh` invokes the exported compiler API directly with Node's TypeScript stripping; `LMSCRIPT_SOURCE` selects another mono checkout. No file in `lmctl-mono` is changed.
- `sdk.lock` pins cpp-dsl-sdk `90e100090534eff869c512156742727cf1723ccb` and the vendored header hash. `scripts/sync-sdk.sh` resolves `origin/main` by default (override `CPP_DSL_SDK_REF`) and `--verify` checks the vendored header.
- Host API declarations use typed v4 registry signatures. `lmcad --dsl-signatures schema/lmcad.dsl.json` is byte-for-byte checked against compiler `--dsl-signatures` output in the test script.
- No renderer, PNG, or mesh-export library is linked. PNG uses stored DEFLATE blocks; PPM/STL/OBJ are written in-tree.

## Build and examples

See the copyable commands in `README.md`. Examples: still-life PNG/PPM/OBJ, 12-facet box STL, range-loop row PNG, and 30-frame CSV keyframe tween. Host result/error lines use the SDK JSON protocol. Argument errors return 2; bytecode load and DSL/host runtime errors return 3.

## Test results

`./test.sh` passes **12 checks**: SDK pin, native build, four v4 compiles, schema/host signature diff, four executions, PNG dimensions and visible pixels, 12-triangle STL, 30 distinct tween frames, PNG CRC/Adler, raster and animation determinism, wrong-schema/bad-argument/camera refusals, and host validation cases.

The ASan+UBSan variant (`BUILD_DIR=build-sanitize ./test.sh` after configuring with sanitizer flags) also passed all **12 checks** on this box. The still-life PNG was visually inspected: the red box, blue sphere and gold cylinder render against the dark background.

## Link checks

The standard Release executable statically links libstdc++ and libgcc; `ldd build/lmcad` should show only libc/libm and the loader. A full static build remains available through `LMCAD_FULL_STATIC=ON`. Measure target-specific size and dependencies for each release platform.
