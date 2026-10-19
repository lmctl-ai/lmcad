# Build, run, and test

## Toolchain and SDK

- Verified on Linux ARM64 with g++ 13.3.0, CMake 3.28.3, and Ninja 1.11.1.
- The current compiler emits bytecode v5 against schema v2. Install `@lmctl-ai/lmctl` so `lmscript` is available on `PATH`, or
  set `LMSCRIPT_BIN` to an explicit compatible compiler. `scripts/select-compiler.sh` probes the schema and a while-loop example;
  it fails with an install hint if no compatible compiler is available.
- `scripts/lmscript-source.sh` is an optional adapter for compiler-source development. It requires an explicit `LMSCRIPT_SOURCE`
  path and imports that compiler without copying it into this repository.
- `sdk.lock` pins the C++ runtime commit and SHA-256 of `cpp/dslvm.hpp`. `scripts/sync-sdk.sh --verify` checks the vendored file.
- CMake embeds the exact schema bytes. `lmcad --schema --json` prints them; plain `--schema` prints a registry-derived summary.
  `--dsl-signatures` prints canonical host declarations for comparison with compiler output.
- `lmcad --dsl A.dslb B.dslb` runs programs in order with the same schema and host state. The old external-schema CLI form is
  not supported.
- OCCT 8.0.1 provides BRep geometry, booleans, fillets, shelling, and meshing. All primitives use OCCT shapes tessellated into the
  existing mesh. In-tree writers handle PNG/PPM/STL/OBJ/APNG; OCCT exchange modules are not linked.

## Build and examples

The root README documents the OCCT prefix build, application build, CLI, and test commands. Examples cover a still life, STL,
procedural loops, CSV tweening, practical solids, a section view, a handled mug, and a camera turntable. Rendering defaults include
key/fill lighting, ambient light, fitted camera framing, and 2x supersampling. The output formats are PNG, PPM, STL, OBJ, and APNG.

## Tests

`./test.sh` checks SDK pinning, the native build, compilation and execution of all examples, schema/signature parity, image and CAD
exports, APNG structure and extracted frames, deterministic output, and invalid-input refusals. The ASan/UBSan build uses
`LMCAD_FULL_STATIC=OFF BUILD_DIR=build-sanitize ./test.sh`.

The section example asserts that its two BRep half-volumes sum to the original 576.0. The polished mug's BRep volume is
344.08933973285542 and its closed STL mesh measures 343.629532 (0.13% lower). The vase BRep volume is 1708.2677036376435;
its STL mesh measures 1739.390003 (1.82% higher). Curved faces are approximated by planar facets at the default 0.05 model-unit
linear and 0.1-radian angular tessellation deflections. The mug uses a sampled smooth profile through the upper body and a
0.35-unit edge fillet. The vase profile is a sampled cubic curve to avoid visible bands.

The in-tree PNG/APNG writer uses deterministic LZ77 and fixed-Huffman DEFLATE, with stored blocks for incompressible data. APNG
uses exact rational frame delays. Tests validate chunk CRCs and sequence, frame count, first-frame identity, changed frames, and
repeatability. Full-canvas frames stream to a temporary file so memory usage does not scale with frame count.

## Static builds

The Linux release executable is fully static; `ldd` reports `not a dynamic executable`. OCCT 8.0.1 builds from source using
`scripts/build-occt.sh`; CI caches the installed prefix by version, configure flags, architecture, and compiler version. Linux
release artifacts include the executable, notices, relinking instructions, and application object files. Platform-specific static
builds for macOS and Windows remain future work.
