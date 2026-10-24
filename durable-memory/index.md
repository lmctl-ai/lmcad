# lmcad durable memory

lmcad is a static lmscript `_dsl_` host for parametric 3D solids: OpenCASCADE 8.0.1 B-rep (booleans, fillets, shells, revolve, extrude), a software rasterizer, and PNG/PPM/APNG/STL/OBJ writers.
Repository: https://github.com/lmctl-ai/lmcad.

## 60-second takeover

Needs a static OCCT prefix (`scripts/build-occt.sh`) and a v5 `lmscript`; see [03-build-run-test.md](03-build-run-test.md) for details.
To run an example against the released host without building from source:
```sh
mkdir -p /tmp/w && cd /tmp/w
lmscript /path/to/lmcad/examples/ball-stick.lms
```
(lmscript fetches `github:lmctl-ai/lmcad@latest` automatically).

## Document map

| File | Contents |
| --- | --- |
| [00-index.md](00-index.md) | Original index and state summary |
| [01-purpose-and-place.md](01-purpose-and-place.md) | Purpose, scope, and design goals |
| [02-render-stack-options.md](02-render-stack-options.md) | Evaluation of rendering backends and CPU rasterizer |
| [03-build-run-test.md](03-build-run-test.md) | Build commands, test execution, and CLI options |
| [03-schema-v0.md](03-schema-v0.md) | The DSL schema specification and type system |
| [04-gotchas.md](04-gotchas.md) | Limits, conventions (radians, axis alignments), and pitfalls |
| [04-loops-and-collections-notes.md](04-loops-and-collections-notes.md) | Loop constructs and collections integration |
| [05-open-items.md](05-open-items.md) | Backlog and planned improvements |
| [06-reference-examples.md](06-reference-examples.md) | Reference CAD examples and engineering scenarios |
| [07-occt-static-spike.md](07-occt-static-spike.md) | OCCT static linking proof and headless operation |
| [08-occt-integration.md](08-occt-integration.md) | Geometry pipeline: shapes, booleans, and tessellation |
| [13-licensing.md](13-licensing.md) | LGPL OCCT relinking obligations and compliance |
| [14-ci-macos-windows.md](14-ci-macos-windows.md) | CI matrix, cross-platform builds, and automatic releases |
| [15-example-gallery.md](15-example-gallery.md) | Gallery catalog, known answers, and image size budgets |
| [20-sibling-3d-hosts.md](20-sibling-3d-hosts.md) | Cross-repo comparison (lmblender, lmcad, lmmagnum, lmfilament) |

## Adding an example or function

### Adding an example
1. Create `examples/<name>.lms` using `_dsl_ cli="lmcad" install="github:lmctl-ai/lmcad@latest"`.
2. Generate its rendered output: `lmscript examples/<name>.lms` (writes `images/<name>.png`).
3. Add the example to `test.sh`.
4. Add the image to `README.md` gallery with `<img src="docs/images/<name>.png" ...>` and `lmctl autorun <raw .lms URL>` caption.

### Adding a host function/method
1. Declare the method/function in the DSL schema (`schema/lmcad.dsl.json`).
2. Implement and register the method in `src/registry.cpp` using the typed `dslvm::Registry` API.
3. Verify that `./test.sh` passes and signatures match `lmscript --dsl-signatures`.

## Releases
Releases are built and published automatically on every push to `main` across Linux (x64/arm64), macOS (arm64/x64), and Windows (x64).
