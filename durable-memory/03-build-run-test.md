# Build, run and test

## Toolchain and SDK

- Checked on Linux ARM64, g++ 13.3.0, CMake 3.28.3, Ninja 1.11.1.
- The current language/compiler/bytecode version is v5 and the schema format is v2. The examples compile with `/home/ubuntu/lmctl-mono/lmscript/dsl.ts` through `scripts/lmscript-mono.sh`; this adapter imports the real compiler and does not edit or copy it. `test.sh` prefers a PATH `lmscript` that accepts schema v2 and compiles a while-loop probe to v5 bytecode; otherwise it falls back to the adapter. `LMSCRIPT_BIN` is an optional explicit override.
- The PATH `lmscript` available here rejects `--dsl-signatures`; all seven examples therefore used `scripts/lmscript-mono.sh`, which imports the real mono `dsl.ts`, and emitted v5 bytecode.
- `sdk.lock` pins cpp-dsl-sdk `b8fd2858a6313ba256b25b6dd250ee2404bc6ef3` and the SHA-256 of the vendored `cpp/dslvm.hpp`. `scripts/sync-sdk.sh` resolves `origin/main` by default; `--verify` checks the copied header.
- CMake embeds the exact schema file bytes in a generated header. `lmcad --schema` prints those bytes; no external schema file is needed at runtime. `lmcad --dsl-signatures` prints the registry's canonical declarations, checked byte-for-byte against the compiler's schema output.
- `lmcad --dsl A.dslb B.dslb` runs both programs sequentially against the same schema, registry, and host context. The removed `--schema FILE` input syntax is rejected.
- OCCT 8.0.1 supplies BRep geometry, booleans, fillets, shelling, and meshing. All solids, including box/sphere/cylinder, use OCCT TopoDS shapes tessellated to the existing mesh. The existing in-tree renderer and PNG/PPM/STL/OBJ writers remain the output path. `TKOffset` is added to the spike 14-library closure for shelling; no OCCT exchange toolkit is linked.

## Build and examples

See the copyable OCCT-prefix and application commands in `README.md`. Examples include still-life PNG/PPM/OBJ, 12-facet box STL, range-loop row, 30-frame CSV keyframe tween, plate-with-holes, hollow brick, and hollow vase. Host result/error lines use the SDK JSON protocol. Argument errors return 2; bytecode load and DSL/host runtime errors return 3.

## Test results

`./test.sh` passes **20 checks**: SDK pin, native full-static build, seven v5 compiles, embedded schema byte identity, host/compiler signature parity, seven example executions, sequential multi-bytecode execution, PNG dimensions/pixels, 12-triangle STL, three CAD STL/PNG exports and CAD volume assertions, 30 distinct tween frames, PNG CRC/Adler, raster/tween/CAD determinism, CLI, camera, and invalid-fillet refusals, host validation cases, three-keyframe boundary/endpoint checks, and relocated adapter parsing. The native unit checks cover CSV quoting/CRLF, complete finite numbers, missing/extra columns, and host row/record limits.

The ASan+UBSan variant (`LMCAD_FULL_STATIC=OFF BUILD_DIR=build-sanitize ./test.sh`) passes all **20 checks**; `LMCAD_FULL_STATIC=OFF` lets the sanitizer runtime link dynamically. A fresh shallow clone of pushed `e113b70` at `/tmp/lmcad-step9-fresh-20261019`, configured with `LMCAD_OCCT_PREFIX=/home/ubuntu/poc/occt-prefix`, builds and passes all **20 checks**. The plate, brick, and vase renders are visually checked in `build/`; their smooth curves and sharp box edges render as expected.

## Link checks

Release uses full static linking. `file build/lmcad` reports statically linked AArch64 ELF, `ldd build/lmcad` reports `not a dynamic executable`, and the Step 9 binary is **26,758,520 bytes (25.5 MiB)**. The clean OCCT source build measured 15:51.52 and 395,208 KiB maximum RSS in the spike; this step reused that build for the prefix install. See `08-occt-integration.md` for operation details and BRep/build123d volume comparisons.
