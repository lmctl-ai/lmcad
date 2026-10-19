# Build, run and test

## Toolchain and SDK

- Checked on Linux ARM64, g++ 13.3.0, CMake 3.28.3, Ninja 1.11.1.
- The current language/compiler/bytecode version is v5 and the schema format is v2. The examples compile with `/home/ubuntu/lmctl-mono/lmscript/dsl.ts` through `scripts/lmscript-mono.sh`; this adapter imports the real compiler and does not edit or copy it. `test.sh` prefers a PATH `lmscript` that accepts schema v2 and compiles a while-loop probe to v5 bytecode; otherwise it falls back to the adapter. `LMSCRIPT_BIN` is an optional explicit override.
- The PATH `lmscript` available here rejects `--dsl-signatures`; all eight examples therefore used `scripts/lmscript-mono.sh`, which imports the real mono `dsl.ts`, and emitted v5 bytecode.
- `sdk.lock` pins cpp-dsl-sdk `b8fd2858a6313ba256b25b6dd250ee2404bc6ef3` and the SHA-256 of the vendored `cpp/dslvm.hpp`. `scripts/sync-sdk.sh` resolves `origin/main` by default; `--verify` checks the copied header.
- CMake embeds the exact schema file bytes in a generated header. `lmcad --schema --json` prints those bytes; plain `--schema` prints a short registry-derived summary with context first and members grouped by type. No external schema file is needed at runtime. `lmcad --dsl-signatures` prints the registry's canonical declarations, checked byte-for-byte against the compiler's schema output.
- `lmcad --dsl A.dslb B.dslb` runs both programs sequentially against the same schema, registry, and host context. The removed `--schema FILE` input syntax is rejected.
- OCCT 8.0.1 supplies BRep geometry, booleans, fillets, shelling, and meshing. All solids, including box/sphere/cylinder, use OCCT TopoDS shapes tessellated to the existing mesh. The existing in-tree renderer and PNG/PPM/STL/OBJ writers remain the output path. `TKOffset` is added to the spike 14-library closure for shelling; no OCCT exchange toolkit is linked.

## Build and examples

See the copyable OCCT-prefix and application commands in `README.md`. Examples include still-life PNG/PPM/OBJ, 12-facet box STL, range-loop row, 30-frame CSV keyframe tween plus APNG, plate-with-holes, hollow brick, hollow vase, scalar-colored 5x5 glyph field, separated section halves, a hollow revolved mug with an extruded loop handle, and a 36-frame APNG turntable. Renders use key/fill lighting, ambient light, scene-wide fit-camera margin, and 2x supersampling by default. Host result/error lines use the SDK JSON protocol. Argument errors return 2; bytecode load and DSL/host runtime errors return 3.

## Test results

`./test.sh` passes **26 top-level checks** (the embedded schema test script reports its own 62 member assertions): SDK pin, native full-static build, eleven v5 compiles/runs, embedded schema byte identity, host/compiler signature parity, sequential multi-bytecode execution, PNG dimensions/pixels, 12-triangle STL, CAD STL topology/volume checks, APNG chunk order/CRC/sequence/delay/frame-count checks, extracted-frame identity, 30 distinct tween frames, raster/APNG/CAD determinism, CLI/runtime refusals, and host validation suites. The section halves sum to 576.0 BRep volume; the mug BRep volume is 357.48602721261472 and its STL is closed (tessellated volume 353.073312, within 2%).

The current stored-DEFLATE animations are `build/tween.apng` (3,409,389 bytes, 30 frames) and `build/turntable.apng` (2,080,334 bytes, 36 frames). The C++ unit parser validates all chunk CRCs, chunk and sequence structure, loop count, dimensions, frame delay, frame count, changed payloads, and exact first-frame IDAT equivalence with `build/tween-0.png`. The extraction utility was used to write and inspect tween frames 0/15/29 and turntable frames 0/18/35.

The ASan+UBSan variant (`LMCAD_FULL_STATIC=OFF BUILD_DIR=build-sanitize ./test.sh`) and fresh shallow clone both pass all **26 checks**. All rendered examples are regenerated. The section rendering shows two offset halves in distinct colors. The mug has a readable open top and C-shaped band handle; compared with the tea-cup reference it is more angular and has a polygonal profile. Extracted tween frames show color/position change; turntable frames show camera orbit.

## Link checks

Release uses full static linking. `file build/lmcad` reports statically linked AArch64 ELF, `ldd build/lmcad` reports `not a dynamic executable`, and the Step 10 binary is **26,762,776 bytes (25.5 MiB)**. The clean OCCT source build measured 15:51.52 and 395,208 KiB maximum RSS in the spike; this step reused that build for the prefix install. See `08-occt-integration.md` for operation details and BRep/build123d volume comparisons.
