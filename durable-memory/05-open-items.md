# Open items

1. Add a configurable camera up vector and expand perspective edge-case fixtures.
2. Decide whether to implement lightweight mesh CSG or evaluate OpenCASCADE as an optional backend if robust Boolean operations become required.
3. Consider a compressed PNG implementation; current stored DEFLATE keeps dependencies out and output deterministic at the cost of file size.
4. Define live versus copied material semantics and use roughness in shading or remove it from a future schema revision.
5. Consider OBJ MTL support and a documented glTF subset if interchange requires them.
6. Add an ffmpeg-free video artifact only if a small, maintainable encoder is justified. Numbered PNG frames are the current portable interchange; no video assembly tool is included.
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
