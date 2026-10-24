# lmcad: start here

lmcad is a static lmscript `_dsl_` host for parametric 3D solids: OpenCASCADE 8.0.1 B-rep (booleans, fillets, shells, revolve, extrude),
a software rasterizer and PNG/PPM/APNG/STL/OBJ writers. Repo: https://github.com/lmctl-ai/lmcad. The numbering below has gaps (09-12) that
never existed; do not look for them.

## 60-second takeover

Needs a static OCCT prefix (`scripts/build-occt.sh`; CI cold builds took 21 min on arm64 and 34 min on x64) and a v5 `lmscript`; see 03-build-run-test.md for the exact commands.
To run an example against the released host without building anything:
`mkdir /tmp/w && cd /tmp/w && lmscript /path/to/lmcad/examples/ball-stick.lms` (lmscript fetches `github:lmctl-ai/lmcad@latest`).
Change flow: schema + host code, rebuild, `./test.sh`, docs, `git commit && git push origin main`.

## Doc map

- 01 purpose and scope; 02-render-stack-options (why a project-owned mesh + CPU rasterizer); 03-build-run-test build/test/CLI; 03-schema-v0 the DSL surface and rationale.
- 04-gotchas (limits, conventions: **rotate is radians**, cylinders run along Y), 04-loops-and-collections-notes, 05-open-items.
- 06-reference-examples (the build123d + pyvista examples a C++ replacement had to cover), 07-occt-static-spike (proof that OCCT links statically and headless) and 08-occt-integration (the geometry path: shapes, booleans, tessellation).
- 13-licensing (LGPL OCCT + relinking obligations), 14-ci-macos-windows (CI, relink assets, automatic releases), 15-example-gallery (every README story, known answers, image budgets).
- 20-sibling-3d-hosts-and-conventions: the four 3D repos side by side, DSL cheat sheet, porting rules, **mandatory working conventions**.

## State (2026-10-24)

38 examples (engineering stories with asserted known answers, plus scenes shared with lmblender, lmmagnum, lmfilament and the four lmctl.com homepage clips).
Released on linux/macos/windows. Open items: 05.
