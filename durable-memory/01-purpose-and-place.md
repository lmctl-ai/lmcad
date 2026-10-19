# Purpose and place

`lmcad` is a native C++ host for lmscript `_dsl_` bytecode. It builds 3D scenes and renders still images, frame sequences, and
APNG animations. A compiled `.dslb` file contains instructions that call host functions and methods; it is not a serialized scene
format.

The CLI embeds its JSON schema, binds its declared functions and objects to native implementations, and owns scene lifetime, file
access, rendering, and process behavior. The compiler checks scripts against the schema. The C++ SDK loads bytecode against the
exact schema bytes and verifies its structure before execution.

The host uses OpenCASCADE for solid geometry and tessellates shapes into an in-tree mesh. A software rasterizer and in-tree PNG,
PPM, STL, OBJ, and APNG writers operate on that mesh. Data input such as CSV keyframes is provided by ordinary host functions;
the VM has no special file or collection support.

## Scope boundary

The host keeps geometry and rendering policy in C++ and exposes a small explicit schema to scripts. Bytecode behavior follows the
compiler/runtime contract and shared conformance vectors; lmcad does not add private instructions or runtime semantics.
