# lmcad

`lmcad` is a headless C++ host for lmscript DSL bytecode. It embeds the exact bytes of [`schema/lmcad.dsl.json`](schema/lmcad.dsl.json), vendors the pinned header-only [`cpp-dsl-sdk`](cpp/dslvm.hpp), and renders PNG/PPM images or exports STL/OBJ meshes.

## Build

Requirements: C++17 compiler, CMake 3.24+, Ninja, and (to compile examples) an lmscript compiler with DSL v5/schema v2 support. CMake generates `schema_embedded.hpp` from the schema file; changing the schema triggers regeneration.

```sh
scripts/sync-sdk.sh --verify
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

`test.sh` prefers `lmscript` on PATH when it accepts schema v2 and compiles a while-loop probe to v5 bytecode. Otherwise it uses `scripts/lmscript-mono.sh`, a small adapter that imports the real `~/lmctl-mono/lmscript/dsl.ts` compiler directly; no compiler copy or environment variable is needed. The adapter compiles standalone `_dsl_` blocks only. Set `LMSCRIPT_BIN` to explicitly select a compiler.

## CLI

```sh
build/lmcad --schema > /tmp/lmcad-schema.json
cmp schema/lmcad.dsl.json /tmp/lmcad-schema.json
build/lmcad --dsl-signatures
scripts/lmscript-mono.sh --dsl-signatures schema/lmcad.dsl.json
scripts/lmscript-mono.sh examples/still-life.lms build/still-life.dslb
build/lmcad --dsl build/still-life.dslb
```

`--schema` prints the embedded schema bytes exactly. `--dsl-signatures` prints the host registry's canonical signature list, which must match the compiler's output for the schema. `--dsl FILE [FILE ...]` executes bytecode files in argument order, using one embedded schema and the same host context for every block. Each file has fresh VM locals, while shapes, materials, camera and other host state persist across files. Execution stops on the first load or runtime error (exit 3); previous outputs and any output already written by the failing block remain. Each completed block prints one success JSON line, followed by an error JSON line if a later block fails. All command-line arguments are validated before execution (invalid syntax exits 2). The old `--schema FILE` input form is removed.

The examples cover a lit still life, STL export, a range-loop row, and CSV-driven tweening. To compile and run them individually:

```sh
scripts/lmscript-mono.sh examples/still-life.lms build/still-life.dslb
scripts/lmscript-mono.sh examples/box-stl.lms build/box-stl.dslb
scripts/lmscript-mono.sh examples/loop-row.lms build/loop-row.dslb
scripts/lmscript-mono.sh examples/tween.lms build/tween.dslb
build/lmcad --dsl build/still-life.dslb
build/lmcad --dsl build/box-stl.dslb
build/lmcad --dsl build/loop-row.dslb
build/lmcad --dsl build/tween.dslb
```

## Checks

```sh
./test.sh
cmake -S . -B build-sanitize -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
BUILD_DIR=build-sanitize ./test.sh
```

The test script checks SDK pinning, v5 example compilation, embedded schema byte identity, compiler/host signature consistency, sequential multi-file execution, PNG contents and determinism, 12-triangle box STL, tween output and host-side validation refusals. The SDK loader rejects every bytecode version except v5. See [`durable-memory/03-build-run-test.md`](durable-memory/03-build-run-test.md) and [`durable-memory/05-open-items.md`](durable-memory/05-open-items.md).
