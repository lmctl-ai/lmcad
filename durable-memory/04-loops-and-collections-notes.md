# Loops and host collections

The language supports `while` loops, integer range loops, and collection iteration. Collection values are ordinary schema object
handles with methods such as `len() -> int` and `at(int) -> T`; they are not VM list or map values. A host can expose a collection
such as `Keyframes`, and a script can iterate with `for (key in keys)` or an explicit range loop.

Range bounds are evaluated once. Loop counters are compiler-owned hidden locals and read-only in the body. The verifier checks
structured loop pairing and lexical control transfers; arbitrary backward jumps are refused. The VM has no execution budget,
resource caps, or dynamic collection limits. Hosts validate their own input and allocations.

lmcad reads CSV keyframes in host code and returns typed `Keyframes` and `Keyframe` handles to the script. The script computes
tween values with ordinary math builtins and asks the scene to render each frame.
