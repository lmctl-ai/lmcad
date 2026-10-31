# lmcad

> The public README, docs, images, and examples for `lmcad` are MIT under `LICENSE-examples`. Binary releases use `LICENSE`; implementation source and its history are maintained privately in `lmctlhq/lmcad`.

**Parametric 3D solids from lmscript: OpenCASCADE B-rep modelling, volume checks, and a built-in software renderer that writes PNG, APNG and STL with no GPU or display.**

[![CI](https://github.com/lmctl-ai/lmcad/actions/workflows/public-ci.yml/badge.svg)](https://github.com/lmctl-ai/lmcad/actions/workflows/public-ci.yml)
[![Latest release](https://img.shields.io/github/v/release/lmctl-ai/lmcad?label=release)](https://github.com/lmctl-ai/lmcad/releases)
[![Examples: MIT](https://img.shields.io/badge/examples-MIT-blue.svg)](LICENSE-examples)

### Machine design

| Spur gears, 2:1 reduction | Compression spring, free vs 200 N | Air receiver cutaway |
| --- | --- | --- |
| <img src="docs/images/gear-train.gif" width="290" alt="Spur gears, 2:1 reduction"> | <img src="docs/images/coil-spring.png" width="290" alt="Compression spring, free vs 200 N"> | <img src="docs/images/pressure-vessel.png" width="290" alt="Air receiver cutaway"> |
| True involute teeth mesh at a = m(z1+z2)/2 = 54 mm<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/gear-train.lms` | Rate k = Gd⁴/8D³n = 11.75 N/mm, so 200 N shortens it 17 mm<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/coil-spring.lms` | ASME wall sizing: 6.6 mm shell, 3.3 mm heads, 396 L checked on the B-rep<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/pressure-vessel.lms` |

### Structures

| Pratt truss footbridge | Same steel, four beam shapes | Plate with eight bores |
| --- | --- | --- |
| <img src="docs/images/truss-bridge.png" width="290" alt="Pratt truss footbridge"> | <img src="docs/images/beam-sections.png" width="290" alt="Same steel, four beam shapes"> | <img src="docs/images/plate-with-holes.png" width="290" alt="Plate with eight bores"> |
| Method of joints: red tension, blue compression, mid top chord −450 kN<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/truss-bridge.lms` | Equal area, but the I-beam deflects 25× less than a solid square<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/beam-sections.lms` | Boolean cuts; the B-rep volume gives 230.5 g of aluminium, 11 % lighter<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/plate-with-holes.lms` |

### Heat, fields and molecules

| How many heat-sink fins? | Electric dipole field | Buckminsterfullerene C60 |
| --- | --- | --- |
| <img src="docs/images/heat-sink.png" width="290" alt="How many heat-sink fins?"> | <img src="docs/images/dipole-field.png" width="290" alt="Electric dipole field"> | <img src="docs/images/buckyball.png" width="290" alt="Buckminsterfullerene C60"> |
| Elenbaas sweep finds 8 fins (4.7 K/W) near the Bar-Cohen spacing<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/heat-sink.lms` | Coulomb sum on a grid; far field matches p/r³ within 0.4 %<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/dipole-field.lms` | 60 atoms from symmetry; 90 bonds pass Euler's V − E + F = 2<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/buckyball.lms` |

### Products

| How much does the mug hold? | Turned vase | 2 × 6 construction brick |
| --- | --- | --- |
| <img src="docs/images/mug.png" width="290" alt="How much does the mug hold?"> | <img src="docs/images/vase.png" width="290" alt="Turned vase"> | <img src="docs/images/brick.png" width="290" alt="2 × 6 construction brick"> |
| Revolve, shell, fillet, fuse a handle: 629.5 mL from B-rep volumes<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/mug.lms` | Bezier profile revolved and shelled: 5.29 L capacity, ceramic volume asserted<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/vase.lms` | Shelled box plus 12 revolved studs: 2.68 g of ABS<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/brick.lms` |

### Scenes shared with lmblender, lmmagnum and lmfilament

| Caffeine molecule | B-DNA double helix | Cornell box |
| --- | --- | --- |
| <img src="docs/images/caffeine-molecule.png" width="290" alt="Caffeine molecule"> | <img src="docs/images/dna-double-helix.png" width="290" alt="B-DNA double helix"> | <img src="docs/images/cornell-box.png" width="290" alt="Cornell box"> |
| Ball-and-stick, 24 atoms in CPK colours (from lmblender)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/caffeine-molecule.lms` | 28 base pairs, 144 degree groove geometry (from lmblender)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/dna-double-helix.lms` | Boxes and a lamp; this renderer has no global illumination (from lmblender)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/cornell-box.lms` |

| Saturn and its rings | Roughness sweep | Material sandbox |
| --- | --- | --- |
| <img src="docs/images/saturn-rings.png" width="290" alt="Saturn and its rings"> | <img src="docs/images/pbr-spheres.png" width="290" alt="Roughness sweep"> | <img src="docs/images/material-sandbox.png" width="290" alt="Material sandbox"> |
| Oblate sphere plus four bored discs, to scale (from lmblender)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/saturn-rings.lms` | Five spheres, roughness 0.05 to 0.95 (from lmmagnum and lmfilament)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/pbr-spheres.lms` | Colour by gloss grid (from lmmagnum and lmfilament)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/material-sandbox.lms` |

| Model viewer | Primitives | Multi draw |
| --- | --- | --- |
| <img src="docs/images/model-viewer.png" width="290" alt="Model viewer"> | <img src="docs/images/primitives.png" width="290" alt="Primitives"> | <img src="docs/images/multidraw.png" width="290" alt="Multi draw"> |
| A posed toy robot from primitives (from lmmagnum and lmfilament)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/model-viewer.lms` | Box, sphere, cylinder, revolved cone and capsule, endpoint tube (from lmmagnum and lmfilament)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/primitives.lms` | 180 meshes in six rings (from lmmagnum and lmfilament)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/multidraw.lms` |

| Cube map | Falling boxes | 3D flow streamlines |
| --- | --- | --- |
| <img src="docs/images/cubemap.png" width="290" alt="Cube map"> | <img src="docs/images/bullet.png" width="290" alt="Falling boxes"> | <img src="docs/images/fluidsimulation3d.png" width="290" alt="3D flow streamlines"> |
| Six-face room around glossy spheres (from lmmagnum and lmfilament)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/cubemap.lms` | 64 boxes with closed-form fall and bounce, no engine (from lmmagnum and lmfilament)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/bullet.lms` | 32 ABC-flow streamlines coloured by speed (from lmmagnum and lmfilament)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/fluidsimulation3d.lms` |

| 2D vortex pair | Checker triangle |   |
| --- | --- | --- |
| <img src="docs/images/fluidsimulation2d.png" width="290" alt="2D vortex pair"> | <img src="docs/images/textured-triangle.png" width="290" alt="Checker triangle"> |   |
| 40 streamlines of two opposite vortices (from lmmagnum and lmfilament)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/fluidsimulation2d.lms` | 64 extruded sub-triangles standing in for a texture (from lmmagnum)<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/textured-triangle.lms` |   |

### Homepage clips from lmctl.com

The four 10 s clips on the [lmctl.com](https://lmctl.com) homepage, one per level of the agent-automation ladder. Their scene scripts live in lmctl-ai/lmctl; the published videos add lmdraw title and label overlays on top of the 3D scenes shown here. Each example writes 240 frames, then ffmpeg encodes `images/<name>.mp4` and `.gif`; the lmcad version is the original storyboard, unchanged apart from output paths.

| Level 1: names and teams | Level 2: agents talk through a Lead |
| --- | --- |
| <img src="docs/images/level-1-sessions.gif" width="440" alt="Level 1: names and teams"> | <img src="docs/images/level-2-orchestration.gif" width="440" alt="Level 2: agents talk through a Lead"> |
| Sessions gather into Lead, Coder and Review rows; a lens inspects one and a usage gauge rises<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/level-1-sessions.lms`<br>[MP4](docs/videos/level-1-sessions.mp4) | Provider-coloured agents orbit the Lead; message packets travel through it<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/level-2-orchestration.lms`<br>[MP4](docs/videos/level-2-orchestration.mp4) |

| Level 3: a hands-off Lead | Level 4: automation |
| --- | --- |
| <img src="docs/images/level-3-hands-off-lead.gif" width="440" alt="Level 3: a hands-off Lead"> | <img src="docs/images/level-4-automation.gif" width="440" alt="Level 4: automation"> |
| A requirement and its checklist enter; a reviewed result returns<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/level-3-hands-off-lead.lms`<br>[MP4](docs/videos/level-3-hands-off-lead.mp4) | A brainstorm becomes a Work, Review, QA and Acceptance pipeline<br>`lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/level-4-automation.lms`<br>[MP4](docs/videos/level-4-automation.mp4) |

Every picture is written by the example named in its caption. Paste the `lmctl autorun` line into a terminal in an empty directory: lmscript downloads the released `lmcad` (and `lmplot` for the three charts), runs the program, and leaves `images/<name>.png` (or `.gif`) there. Each example also asserts its own answer (a volume, a force, a count) on the host, so a wrong model fails instead of rendering. `examples/render-all.sh` regenerates `docs/images/`.

## Quick start (1 minute)

```sh
npm install -g @lmctl-ai/lmctl        # lmscript + lmctl (Node.js >= 24)
export GITHUB_TOKEN=$(gh auth token)  # optional: avoids GitHub API rate limits on first install
lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/gear-train.lms
```

A program is an ordinary lmscript file with a `_dsl_` block that names the host. The block is compiled against the schema the `lmcad` binary reports, saved as bytecode, and run by that binary:

```
out = ARGV.len() > 0 ? ARGV[0] : "private implementation detail";
mkdir("images");
mkdir("build");
p = _lmcode_ cli="lmcad" install="github:lmctl-ai/lmcad@latest";
let shape = scene.box(2.0, 3.0, 4.0, material().color(0.2, 0.55, 0.85));
scene.assert_close(shape.volume(), 24.0, 0.000001);
scene.fit_camera(42.0, 1.2);
scene.render_png("images/box.png", 640, 480);
end _lmcode_
savedsl(p, out);
r = system(concat(dsl_cli(p), ["--lmcode", out]), 600000);   # run with the same host; system() defaults to a 60 s limit
if (r.code != 0) { println(r.stderr + r.stdout); }
assert(r.code == 0);
println("wrote images/box.png");
```

`lmcad --lmcode-schema` lists the whole API: primitives (box, sphere, cylinder, `tube_between`), sketches revolved or extruded into solids, booleans (`union`, `cut`, `intersect`), `fillet`/`chamfer`/`hollow`, B-rep `volume()` and bounds, lights, cameras, supersampling, PNG/PPM/APNG renders and STL/OBJ export. More basic examples (still life, STL export, range loop, keyframe tween, turntable, section view, water molecule) are listed in [examples/README.md](examples/README.md), each with its autorun line. Guides: [tool book](https://lmctl.com/tools/lmcad/) · [get started](https://lmctl.com/tools/lmcad/get_started.html) · [API guide](https://lmctl.com/tools/lmcad/guide.html) · [releases](https://github.com/lmctl-ai/lmcad/releases).

## Limits (honest)

- The renderer is a deterministic z-buffer rasteriser with diffuse plus Blinn-Phong shading: no shadows, reflections, transparency or text. Labels and charts come from a second host (the truss, beam and heat-sink figures use lmplot).
- Sketches are straight-segment polygons, so arcs and involutes are approximated by points; there is no sweep along a path, loft, cone or torus (the spring is built from short tubes).
- Host limits: sketch coordinates within ±500 units, part extent 1000, 2000 faces and 10,000 edges per shape, 1,000,000 triangles, 240 animation frames and 64 million total frame pixels. Model in mm, cm or m to stay inside them.
- Inside a `_dsl_` block the language has `let`, `if`, `for`/`range`, arithmetic and `sin cos sqrt pow abs floor min max clamp lerp ease`; there are no arrays, user functions or `atan`, and outer lmscript variables are not visible.
- Exchange formats are write-only STL/OBJ; STEP/IGES import and export are not built in.

## CLI

```sh
private implementation detail --lmcode-schema
private implementation detail --lmcode-schema --json > /tmp/lmcad-schema.json
cmp private implementation detail /tmp/lmcad-schema.json
private implementation detail --lmcode-signatures
lmscript --lmcode-signatures private implementation detail
PATH="$PWD/build:$PATH" lmscript examples/still-life.lms   # cli="lmcad" resolves to the build
private implementation detail --lmcode private implementation detail                      # rerun the saved bytecode
```

`--version` prints `lmcad X.Y.Z` (the CMake project version; a release tag must be `vX.Y.Z`, which CI checks, because lmscript `install="github:lmctl-ai/lmcad@latest"` compares the two). `--lmcode-schema` prints a compact registry-derived summary with context first, then free functions and members grouped by type. `--lmcode-schema --json` prints the embedded schema bytes exactly. `--lmcode-signatures` prints the host registry's canonical signature list, which must match the compiler's output for the schema. `--lmcode FILE [FILE ...]` executes bytecode files in argument order, using one embedded schema and the same host context for every block. Each file has fresh VM locals, while shapes, materials, camera and other host state persist across files. Execution stops on the first load or runtime error (exit 3); previous outputs and any output already written by the failing block remain. Each completed block prints one success JSON line, followed by an error JSON line if a later block fails. All command-line arguments are validated before execution (invalid syntax exits 2). The old `--lmcode-schema FILE` input form is removed.

`scene.animation(path, w, h, fps)` writes APNG; the gear-train and turntable examples convert theirs to GIF with ffmpeg. PNG and APNG use deterministic LZ77/fixed-Huffman DEFLATE (stored blocks for incompressible data). APNG uses exact rational frame delays (integer FPS from 1 through 240) and loops forever; `lmcad-apng-extract` extracts a zero-based full frame to PNG. Host limits are 240 frames, 64 million total frame pixels, and 16,777,216 pixels per frame. Frames stream to a temporary file in the destination directory; memory does not grow with frame count. `finish()` atomically replaces the destination. Omitting it discards the temporary animation when the host exits; an empty finish, second finish, or add after finish is an error. Write failures preserve the previous destination and remove temporary output. Frame-count and cumulative-pixel checks precede rendering; the renderer also checks the supersampled dimensions and pixel count before allocating. A 36-frame 1920x1080 animation exceeds the cumulative limit (74,649,600 pixels); at that resolution at most 30 frames fit, with supersampling at most 2. `fit_camera(fov, margin)` snapshots visible bounds and adapts the fitted distance to each render’s aspect ratio; FOV is vertical and margin must be at least 1. Explicit camera `look_at` or `clipping` calls disable automatic fitting. Ambient must be finite and nonnegative; values above 1 clamp to full ambient illumination. Scene settings provide key and fill lights, ambient level, and bounded 1..8 supersampling (default 2). OCCT tessellation uses analytic surface normals, blends them across nearly tangent face seams, and keeps sharper B-rep creases crisp. The default mesh uses 0.05 linear deflection and 0.1-radian angular deflection. Materials support bounded Blinn-Phong highlights through `specular(strength)` and `shininess(power)`; key and fill directions are tuned for stronger face contrast. Boolean operations and solid-volume assertions use the BRep geometry.

`scene.tube_between(x1, y1, z1, x2, y2, z2, radius, material)` creates a solid cylindrical segment between two 3D endpoints. It is useful for molecule bonds, rigid-body links, and trajectory tubes without requiring callers to calculate rotations.

## Test

```sh
./test.sh
cmake -S . -B build-sanitize -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' \
  -DLMCAD_OCCT_PREFIX="$PWD/private implementation detail" -DLMCAD_FULL_STATIC=OFF
LMCAD_FULL_STATIC=OFF BUILD_DIR=build-sanitize ./test.sh
```

The test script checks SDK pinning, that every example self-executes against the built host (lmplot examples are skipped with a message when lmplot is unavailable), that every README autorun target and gallery image exists and is a valid PNG/GIF, JSON schema byte identity, complete summary/signature consistency, sequential multi-file execution, PNG contents and determinism, bounded supersampling, STL topology/volume, CAD exports and assertions, APNG chunk order/CRCs/sequences/delays/frame counts, extracted-frame identity, changed frames, and host-side validation refusals. The SDK loader rejects every bytecode version except v5. See `private implementation detail`, `private implementation detail`, and `private implementation detail`.

Linux CI builds static OCCT once per runner architecture/toolchain cache key, then builds the fully static host and runs `test.sh`. The verified cold run produced 30,116,488-byte x64 and 26,767,080-byte arm64 executables; warm jobs passed all CI checks and `ldd` reported `not a dynamic executable`. The current release lane publishes `lmcad-linux-x64` and `lmcad-linux-arm64` with the license, notices, and OCCT relinking materials. Earlier releases may contain macOS and Windows assets; those platforms are not produced by the current lane. See `private implementation detail` and `private implementation detail` before redistributing binaries.

Supersampling uses an n×n box filter over all subpixels, decoding sRGB bytes to linear light for averaging and encoding the result back to sRGB. Output dimensions stay unchanged, including odd sizes. Both raster dimensions after multiplication by n must be at most 8192, with at most 16,777,216 high-resolution pixels; this bounds the RGB/depth buffers before allocation. The renderer uses a diffuse plus Blinn-Phong display-colour model; it is not physically based.

## License


Releases up to and including v0.1.72 were published under the MIT License (see the LICENSE asset of that release). Releases after v0.1.72 are published under the Lattice Systems Binary License 1.0 (this file).
The README, docs, images, and `examples/*.lms` are MIT-licensed under `LICENSE-examples`.
Binary releases are licensed under `LICENSE`. Third-party components remain under their own licenses and notices.
