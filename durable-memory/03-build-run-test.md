# Build, run, and test

## Toolchain and SDK

- Verified on Linux ARM64 with g++ 13.3.0, CMake 3.28.3, and Ninja 1.11.1.
- The current compiler emits bytecode v5 against schema v2. Install `@lmctl-ai/lmctl` so `lmscript` is available on `PATH`, or
  set `LMSCRIPT_BIN` to an explicit compatible compiler. `scripts/select-compiler.sh` probes the schema and a while-loop example;
  it fails with an install hint if no compatible compiler is available.
- `scripts/lmscript-source.sh` is an optional adapter for compiler-source development. It requires an explicit `LMSCRIPT_SOURCE`
  path and imports that compiler without copying it into this repository.
- `sdk.lock` pins the C++ runtime commit and SHA-256 of `cpp/dslvm.hpp`. `scripts/sync-sdk.sh --verify` checks the vendored file.
- `lmcad --version` prints `lmcad <project VERSION from CMakeLists.txt>`. CI stamps the release version into CMakeLists.txt before building (automatic releases, see 14-ci-macos-windows.md), so do not bump it by hand; the release job still fails when the tag and `--version` differ (lmscript `@latest` keeps a cached binary only while they agree).
- CMake embeds the exact schema bytes. `lmcad --dsl-schema --json` prints them; plain `--dsl-schema` prints a registry-derived summary.
  `--dsl-signatures` prints canonical host declarations for comparison with compiler output.
- `lmcad --dsl A.dslb B.dslb` runs programs in order with the same schema and host state. The old external-schema CLI form is
  not supported.
- OCCT 8.0.1 provides BRep geometry, booleans, fillets, shelling, and meshing. All primitives use OCCT shapes tessellated into the
  existing mesh. In-tree writers handle PNG/PPM/STL/OBJ/APNG; OCCT exchange modules are not linked.

## Build and examples

The root README documents the OCCT prefix build, application build, CLI, and test commands. Rendering defaults include
key/fill lighting, ambient light, fitted camera framing, and 2x supersampling. The output formats are PNG, PPM, STL, OBJ, and APNG.

## Examples and README images (2026-10-23)

All examples in `examples/*.lms` are self-executing (the lmplot shape): `out = ARGV...; mkdir("images"); mkdir("build");
p = _dsl_ cli="lmcad" install="github:lmctl-ai/lmcad@latest"; ... end _dsl_ savedsl(p, out);
r = system(concat(dsl_cli(p), ["--dsl", out]), 600000); if (r.code != 0) { println(r.stderr + r.stdout); } assert(r.code == 0);`.
The function form matters: lmscript `system` kills the child after 60 s by default (code null, timed_out true), which failed
buckyball on the slower linux-arm64 runner, and the shorthand hides the host's error JSON. A missing executable makes `system`
throw (spawn ENOENT) rather than return a code, so optional tools are guarded with `which("ffmpeg") != null`. Pictures go to `./images/<name>.png|gif`; STL, PPM, APNG, frames and `keyframes.csv` (tween writes it
with `write_file`) go to `./build/`. No example reads repository files, so `lmctl autorun <raw URL>` works from an empty directory.
There are 12 story examples (README gallery, grouped by theme) and 8 basics (examples/README.md); see 06-reference-examples.md.

- Three examples combine hosts: `truss-bridge`, `beam-sections`, `heat-sink` render a PPM with lmcad, then an lmplot block plots it
  with `binary array=(W,H) skip=15 flipy format='%uchar' ... with rgbimage` beside a chart. DSL blocks cannot see outer or other
  blocks' variables, so each block recomputes the numbers it needs (same formulas).
- Animations: `scene.animation()` writes APNG to build/, then the outer script runs
  `ffmpeg -i x.apng -vf "split[a][b];[a]palettegen=max_colors=96[p];[b][p]paletteuse=dither=none" -loop 0 images/x.gif`
  and prints a skip message if ffmpeg is missing (gear-train, turntable). gear-train is a seamless 36-frame loop.
- Every story asserts its known answer on the host (`scene.assert_close`): volumes, capacities, forces, spring rate, bond count.
- `examples/render-all.sh` runs every example from the repo root (local build via PATH or `LMCAD_BIN`, lmplot from the lmscript
  cache or GITHUB_TOKEN install, ffmpeg required) and copies the gallery set into `docs/images/` with size checks
  (PNG <= 300 KB, GIF <= 3 MB, ~900 px wide). `docs/readme/` (hero/architecture art) and `examples/renders/` were removed.
- Host limits met while modelling: sketch coordinates <= 500 units (pressure vessel is modelled in cm), extent 1000;
  `hollow`/`hollow_top` need a planar end face (closed capsules are shelled with a boolean cut instead).
- Runtime: the coil spring (~550 tube/sphere shapes) is the slowest, ~20 s CPU; everything else is under 10 s CPU.

## Tests

`./test.sh` checks SDK pinning, the native build, schema/signature parity, then runs every example through lmscript with
`build/` first on PATH (so `cli="lmcad"` binds the host under test), checks the images they write, verifies every README
`lmctl autorun` target and `docs/images` file (PNG/GIF magic bytes, at least 12 each), and rejects "see source" links. lmplot
examples are skipped with a message when lmplot is not on PATH/in the lmscript cache and no GITHUB_TOKEN is set (CI passes the
workflow token; Windows skips them). It also checks image and CAD exports, APNG structure and extracted frames, deterministic
output, and invalid-input refusals. tests/tween.py runs only the tween example's _dsl_ block against the schema file;
tests/data/keyframes.csv is the fixture for tween/refusal tests. The ASan/UBSan build uses
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
