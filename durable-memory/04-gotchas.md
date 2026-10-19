# Gotchas and limits

- Schema bytes are part of bytecode binding. Recompile after any schema edit. CMake embeds `schema/lmcad.dsl.json` into the
  executable.
- Schema v2 represents each function and method as an array of overload signatures, including single-overload members. Keep
  registry signatures aligned with compiler `--dsl-signatures` output.
- Compiler selection requires a compatible v5 `lmscript` on `PATH` or an explicit `LMSCRIPT_BIN`. Install
  `@lmctl-ai/lmctl` for the published compiler. The optional source adapter requires explicitly setting `LMSCRIPT_SOURCE`.
- Collections are host object handles with methods such as `len()` and `at(i)`. Scripts iterate with range loops; bytecode has no
  list or map value types.
- The host validates geometry inputs, camera presence, image dimensions, frame totals, and CSV data. The VM has no execution,
  memory, or collection budgets. A nonterminating script can run indefinitely.
- CSV columns are `t,x,y,z,r,g,b`; quoted fields and CRLF are accepted. Records are limited to 4096 bytes and the file to 4096
  rows. Values must parse completely and be finite, times must increase, color channels are in [0,1], and at least two rows are
  required. Tween segments share endpoints once and use equal frame counts regardless of timestamp spacing.
- Host object handles are owned by the process. Mutators update existing shapes, allowing scripts to reuse a shape across frames.
  Multiple bytecode files in one invocation share host state.
- Boxes and cylinders are centered at the origin; cylinders run along Y. `Sketch.point(x,y)` creates a 2D profile, `extrude`
  advances along Z, and `revolve_y` revolves about Y. `hollow` removes the lowest-Y planar face; `hollow_top` removes the highest.
  Boolean and finishing operations bake transforms into the BRep and reset display transforms.
- Tessellation averages curved-face normals across edges within about a 26-degree threshold; sharp box corners remain sharp. Shading is
  a simple software model with ambient fill, no shadows, transparency, or textures.
- STL omits appearance. OBJ contains vertices and faces but no MTL. Neither export requires a camera.
- OCCT 8.0.1 is linked from 15 static archives, including `TKOffset` for shelling. The host uses in-tree mesh exporters rather
  than OCCT exchange toolkits. Validate each target operating system and toolchain independently.
- Repeated booleans retessellate at linear deflection 0.05. Combine related operations before rendering when possible; tessellation
  is serial to keep output deterministic.
