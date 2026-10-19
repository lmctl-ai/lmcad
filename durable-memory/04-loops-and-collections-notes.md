# V4 loops and host collections

This note supersedes the initial v3 planning in this file's history. Compiler and C++ SDK v4 loop support is available in the current sources. lmcad uses `for (i in range(a, b))`, `lerp`, and `ease` for procedural scene generation and animation.

Per operator decisions R31-R34:

- Collections are schema object types, not bytecode LIST/MAP types. A host collection exposes methods such as `len()` and `at(i)`; iteration uses `for (i in range(0, xs.len()))`.
- No bytecode/VM security limits, execution budgets, or resource caps are to be added. Host applications may validate their own inputs and allocations.
- Bytecode v4 uses int64, structured `FOR_RANGE`/`FOR_NEXT`, and math builtins including `lerp`/`ease`.
- Keep lmcad schema at v1 while the current compiler's schema parser accepts that protocol. Schema v2 signature overload arrays belong to a later compiler slice.

lmcad's CSV collection is host-owned. The VM sees only typed `Keyframes` and `Keyframe` object handles declared in the schema.
