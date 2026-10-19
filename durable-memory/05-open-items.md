# Open items

1. Add a configurable camera up vector and expand perspective edge-case fixtures.
2. Decide whether to implement lightweight mesh CSG or evaluate OpenCASCADE as an optional backend if robust Boolean operations become required.
3. Consider a compressed PNG implementation; current stored DEFLATE keeps dependencies out and output deterministic at the cost of file size.
4. Define live versus copied material semantics and use roughness in shading or remove it from a future schema revision.
5. Consider OBJ MTL support and a documented glTF subset if interchange requires them.
6. Add an ffmpeg-free video artifact only if a small, maintainable encoder is justified. Numbered PNG frames are the current portable interchange; no video assembly tool is included.
7. Add per-OS CI builds and record binary size, static-link result, and shared-library audit for each published target.
8. Expand raster tests for clipping, overlapping surfaces, and camera edge cases.
9. The current in-tree compiler accepts schema v1 but not the schema v2 overload-array form in the draft. Repro: change `"dsl.schema": 1` to `2` in `schema/lmcad.dsl.json` and invoke `scripts/lmscript-mono.sh --dsl-signatures schema/lmcad.dsl.json`; it rejects with `"dsl.schema" must be 1; found 2`. This is tracked as later compiler/schema work, not an lmcad bytecode-v4 blocker; do not modify `lmctl-mono` in the lmcad step.

## Upstream SDK policy mismatch found during review

The pinned SDK `90e1000` has no resource budgets, but still performs runtime value-kind checks in `Program::run` (for example, `CALL_METHOD` receiver validation in `cpp/dslvm.hpp:377` and arithmetic kind checks). These conflict with the operator's stated R33/R34 exclusion of dynamic VM checks. Resolve this in the shared SDK and then refresh the pin; this host review preserves the verbatim pinned header. All new CSV and dimension checks are host-side.
