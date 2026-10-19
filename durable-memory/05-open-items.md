# Open items

1. Add a configurable camera up vector and expand perspective edge-case fixtures.
2. Improve camera up-vector control and consider anti-aliasing after the new CAD path is stable.
3. Consider a compressed PNG implementation; current stored DEFLATE keeps dependencies out and output deterministic at the cost of file size.
4. Define live versus copied material semantics and use roughness in shading or remove it from a future schema revision.
5. Consider OBJ MTL support and a documented glTF subset if interchange requires them.
6. APNG is now the ffmpeg-free animated output. It stores full frames with uncompressed DEFLATE and can be large; consider a compact DEFLATE implementation or frame-delta encoding only if output size becomes a practical problem. `Animation.add_frame()` snapshots the scene, and host limits currently cap one animation at 240 frames and 64 million cumulative frame pixels.
7. Add per-OS CI builds and record binary size, static-link result, and shared-library audit for each published target.
8. Expand raster tests for clipping, overlapping surfaces, and camera edge cases.
9. **SDK R33/R34 runtime-check inventory (cpp-dsl-sdk `b8fd285`).** The SDK still has defensive runtime checks in `Program::run`: operand-stack underflow in `pop()`/`DUP`/`POP`; read-before-assignment on local loads; a missing context pointer; method receiver kind/type; `EQ`/`NE` kind agreement; int and float opcode operand kinds; int/float negation and `float()` conversion kinds; `CONCAT` and `str()` input kinds; `!` and conditional-jump bool kinds; and int division/modulo by zero. `Reader::seek()` also checks a negative/out-of-range jump destination during execution. Verified compiler output should make the kind, stack and local guards unreachable; zero divisors remain a language-defined runtime error. The loader separately performs the structural/static bytecode, import, loop-pairing, stack-depth, and version checks required before execution.
10. SDK `RunStats` counts dispatched instructions when requested, but does not enforce an instruction limit. The VM has no execution budget, stack cap, local cap, or string-size cap. Host checks in lmcad (finite/positive geometry inputs, image-size limits, CSV validation and the current 4096-object/row limits) remain host policy and are not VM checks.

### Minimal repro for an extant VM runtime check

Compile this valid chunk against `schema/lmcad.dsl.json` and run it with a host that registers the schema but intentionally omits `context.set("scene", ...)`:

```lmscript
p = _dsl_ schema="lmcad.dsl.json";
scene.background(0.0, 0.0, 0.0);
end _dsl_
savedsl(p, "missing-context.dslb");
```

`Program::run` returns a runtime error that the host did not provide `scene`. This is an SDK defensive check against an invalid host invocation, and illustrates the remaining mismatch with the operator's no-dynamic-VM-check direction. The schema's bytecode `LOAD_CTX` type and context registry binding are statically verified. No change was made to `cpp-dsl-sdk` or its conformance suite in this step; discuss with the SDK owner whether/when to remove the run-time guards after the language contract stabilizes.

### DSL friction noted in Step 9

The array-style hole/stud loops work, but arithmetic requires explicit numeric conversion in every range-derived coordinate:

```lmscript
let x = -20.0 + float(ix) * 8.0;
let z = -4.0 + float(iz) * 8.0;
```

A more convenient form would allow safe widening of `int` to `float` in mixed numeric arithmetic, so the same code could write `-20.0 + ix * 8.0`. No compiler or SDK change was made; current scripts keep the explicit casts.

### Step 11 notes

- Added `section-view.lms`; the two colored, separated half-solids are made with box intersections. Their BRep volumes sum to the original solid's volume. A true infinite half-space primitive and section-face material override would make arbitrary section cuts more direct.
- Added `mug.lms`; its body is revolved, shelled, and filleted. A polygonal C-shaped sketch is extruded and fused as the handle. Its exported STL is closed and consistently oriented. Compared with `reference/out/b3d_05_tea_cup/b3d_05_tea_cup.png`, this has a squared band handle and simpler vessel profile; a true arc/sweep and smoother sketch curves would bring it closer to the rounded reference handle.
- `tween.lms` now writes a 30-frame APNG as well as numbered PNGs; `turntable.lms` writes 36 APNG camera views. APNG frames are standalone full canvases and use stored DEFLATE, favoring a simple deterministic writer over small output size.
- No new compiler friction was found; no changes were made to the compiler, SDK, or other hosts.

### Step 10 notes

- Added a scene-wide `fit_camera(fov, margin)` helper over visible shape bounds, key/fill directional lights, ambient level, and deterministic 1..4 supersampling (host default 2). Supersampled dimensions still use the rasterizer's 16,777,216-pixel internal limit; at factor 4 this means output images may need to stay at or below 1,048,576 pixels.
- Step 10's glyph-field example uses loops and script-side `lerp()` for the scalar color ramp. That DSL was readable; no new friction found. Existing loop arithmetic still requires explicit `float(i)` casts (see Step 9 note).
- The glyph example uses spheres colored by a scalar over a 5x5 grid, inspired by the ranked PyVista glyph reference. It has no single meaningful solid volume because each sphere is a separate instance.
- Plain `lmcad --schema` now mirrors lmhtml's compact, registry-derived format; `--schema --json` returns the exact embedded schema bytes.
