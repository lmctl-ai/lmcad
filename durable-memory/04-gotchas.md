# Gotchas and limits

- **Schema bytes are part of bytecode binding.** Recompile after any schema edit; the loader refuses a hash mismatch. The current compiler implementation accepts schema version 1 declarations, with one parameter/return signature per name.
- **V4 compiler selection.** The global npm `lmscript` is still v3 here. Use `scripts/lmscript-mono.sh`, which imports `dsl.ts` without installing into or editing the mono checkout. The current TypeScript source has no schema v2 overload-array support yet; that work is a later compiler slice. This lmcad schema stays v1 while using v4 bytecode loops, int64, and math builtins.
- **Collections follow R31.** Keyframes are host object handles with `len()` and `at(i)`, and DSL iterates `for (i in range(0, keys.len()))`. There are no bytecode list/map values. VM resource budgets and runtime caps are excluded by R33/R34; the CSV reader's row limit and scene/image validation are host choices.
- **Camera is required to render.** Output width/height must be positive, at most 8192 each, and at most 16,777,216 pixels total. These remain host-side application rules.
- **CSV shape.** Header is exactly `t,x,y,z,r,g,b`; times must increase, channels are in [0,1], and at least two rows are needed. The sample tween currently emits 30 evenly sampled frames per segment.
- **Host object lifetime.** Handles are owned by the host for one run. Shape mutators update the existing host object, so tween scripts can reuse one shape across frames.
- **Primitive/render limits.** Meshes are faceted; shading is flat Lambert plus ambient. There is no anti-aliasing, shadow map, transparency, texture mapping, or CSG.
- **PNG prioritizes self-containment.** Stored DEFLATE is valid but uncompressed, so images are larger than compressed output. Tests inspect PNG signature, IHDR, CRC-32, Adler-32, and deterministic bytes.
- **Exports are mesh-level.** STL drops appearance; OBJ has vertices/faces but no MTL. Neither export requires a camera.
- **Static means target-specific.** Full static linking was supported in the initial ARM64 check; validate each target OS/toolchain and libc compatibility independently.
