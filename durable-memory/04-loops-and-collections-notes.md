# Loops and collections: notes for a future bytecode v4

This is a coordination note only. Do not change `cpp-dsl-sdk` or lmscript as part of the initial lmcad investigation. Bytecode and SDK are currently version 3; the existing verifier accepts only forward jumps, so every accepted chunk terminates. lmcad's first still-image milestone does not require iteration.

## Why a coordinated v4 is needed

Loops change the verifier's termination guarantee. Lists and maps change the portable value model, compiler type checker, VM operations and host API. These cannot be an lmcad-only extension: compiler, language spec, C++ and any other SDKs, and shared conformance vectors must agree. The current `Value` kinds are VOID, INT, FLOAT, STR, BOOL and OBJ, with raw host object handles.

## Candidate loop design

- Start with a structured, bounded `for i in range(start, end)` form, half-open range and documented iteration direction. Avoid arbitrary `while` for the first loop milestone unless a real use case requires it.
- Compiler lowers structured loops to conditional and unconditional branches. The verifier must accept valid backward edges while still checking every target lands on an instruction boundary, operands/locals remain in bounds, and control flow cannot escape the code region.
- Add a runtime instruction/step budget per `Program::run`, configurable by the embedding host within a hard maximum. Define whether each opcode or each loop iteration consumes budget; opcode count is simpler and closes non-loop infinite paths too. Budget exhaustion is a deterministic DSL runtime error.
- Decide how the budget interacts with nested calls (currently host calls are synchronous callbacks, not DSL calls), and whether a host can set a budget per run or only per program.
- Add vectors for valid zero/one/many-iteration loops, nested loops, loop-local values, boundary/overflow ranges, backward jump accepted only in a valid loop, malformed backward control flow, infinite-loop/step exhaustion, and budget reset across repeated runs.

## Candidate collection model

- Define abstract `LIST<T>` and `MAP<K,V>` types and the supported key types before adding opcodes. A conservative first map key is STR; allowing INT and STR needs canonical equality/hash semantics across SDKs.
- Specify literal, construction, indexing, length, mutation, iteration order, equality, missing-key behavior, and out-of-range behavior. Prefer explicit host-neutral operations; do not expose STL or language-native collection object layout in bytecode.
- Decide whether collections are VM-owned values or typed host handles. VM-owned values simplify portable literals/operations; handles simplify large inputs and host integration but complicate ownership, conformance and type checking. Given the intended abstract value semantics, VM-owned collections are the clearer starting hypothesis, subject to bounded allocation limits.
- Add allocation, length, nesting and string/key size limits. State copy/reference behavior and whether list/map values can be passed to host callbacks. Map iteration needs a specified deterministic order for cross-SDK traces and reproducible scripts.
- Extend conformance manifest and vectors to cover list/map creation and operations, type mismatches, empty collections, indexing and mutation errors, limit checks, and deterministic map traversal.

## Required coordinated work

1. Update the normative language and bytecode specs first: syntax, types, opcodes/encoding, versioning and error semantics.
2. Bump bytecode `VERSION` to 4 in compiler and SDK together. The current policy rejects other versions rather than maintaining backward compatibility; regenerate v4 artifacts and explicitly decide the fate of v3 fixtures.
3. Update compiler type checking and emission, SDK loader/verifier/VM and resource limits, then regenerate language-neutral vectors from the compiler.
4. Update each SDK conformance runner and host `sdk.lock`; run the same vector set on every implementation. Refresh host schema fixtures only if collection types are allowed in schemas.
5. Add bounded loop/collection examples, including an lmcad frame-generation example, after core semantics pass conformance.

## Open questions

- Is the loop syntax only `for i in range(a,b)`, or should iteration over collections be part of the same release?
- Does `range` permit negative bounds and descending steps? Are endpoints int32 and how is overflow handled?
- What is the default step budget, how is it configured, and does it count host-call instructions?
- Are collection literals required, or is host-provided collection input sufficient for v4?
- Which element/key types are legal, and are nested collections supported?
- Are collections mutable? If so, are assignments by value or reference, and how are aliases observed by host callbacks?
- Is MAP iteration insertion ordered, key sorted, or intentionally unspecified? Cross-runtime deterministic behavior favors a specified order.
- What collection and total VM memory limits are portable across constrained targets?
- Should v4 be a one-time coordinated cutover (consistent with current no-backward-compatibility policy), or should SDKs temporarily read v3 and v4?
- Does the compiler-generated conformance manifest need a typed-value description to let host runtimes implement collection callbacks consistently?
