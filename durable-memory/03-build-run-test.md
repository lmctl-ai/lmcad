# Build, run and test

## Toolchain and SDK

- Checked on Linux ARM64, g++ 13.3.0, CMake 3.28.3, Ninja 1.11.1.
- The current language/compiler/bytecode version is v5 and the schema format is v2. The examples compile with `/home/ubuntu/lmctl-mono/lmscript/dsl.ts` through `scripts/lmscript-mono.sh`; this adapter imports the real compiler and does not edit or copy it. `test.sh` prefers a PATH `lmscript` that accepts `--dsl-signatures` for schema v2; otherwise it falls back to the adapter. `LMSCRIPT_BIN` is an optional explicit override.
- At this resync, PATH `lmscript` was 0.2.333 and rejected `--dsl-signatures`; all four examples therefore used the mono `dsl.ts` adapter and emitted v5 bytecode.
- `sdk.lock` pins cpp-dsl-sdk `b8fd2858a6313ba256b25b6dd250ee2404bc6ef3` and the SHA-256 of the vendored `cpp/dslvm.hpp`. `scripts/sync-sdk.sh` resolves `origin/main` by default; `--verify` checks the copied header.
- CMake embeds the exact schema file bytes in a generated header. `lmcad --schema` prints those bytes; no external schema file is needed at runtime. `lmcad --dsl-signatures` prints the registry's canonical declarations, checked byte-for-byte against the compiler's schema output.
- `lmcad --dsl A.dslb B.dslb` runs both programs sequentially against the same schema, registry, and host context. The removed `--schema FILE` input syntax is rejected.
- No renderer, PNG, or mesh-export library is linked. PNG uses stored DEFLATE blocks; PPM/STL/OBJ are written in-tree.

## Build and examples

See the copyable commands in `README.md`. Examples: still-life PNG/PPM/OBJ, 12-facet box STL, range-loop row PNG, and 30-frame CSV keyframe tween. Host result/error lines use the SDK JSON protocol. Argument errors return 2; bytecode load and DSL/host runtime errors return 3.

## Test results

`./test.sh` passes **16 checks**: SDK pin, native build, four v5 compiles, embedded schema byte identity, host/compiler signature parity, four example executions, sequential multi-bytecode execution, PNG dimensions/pixels, 12-triangle STL, 30 distinct tween frames, PNG CRC/Adler, raster and animation determinism, removed-schema-option/bad-argument/camera refusals, host validation cases, three-keyframe boundary/endpoint checks, and relocated adapter parsing. The native unit checks cover CSV quoting/CRLF, complete finite numbers, missing/extra columns, and host row/record limits.

The ASan+UBSan variant (`BUILD_DIR=build-sanitize ./test.sh`) passes all **16 checks**. A shallow fresh clone of pushed `60c6107` under `/tmp/lmcad-step8-8YS7cC` also builds and passes all **16 checks**, including rendering every example. The still-life PNG was visually inspected: the red box, blue sphere, and gold cylinder are visible against the configured dark background. The loop-row render shows the box row; tween frames 0, 14, and 29 show red, intermediate purple, and blue sphere positions/colors.

## Link checks

The standard Release executable statically links libstdc++ and libgcc; `ldd build/lmcad` should show only libc/libm and the loader. A full static build remains available through `LMCAD_FULL_STATIC=ON`. Measure target-specific size and dependencies for each release platform.
