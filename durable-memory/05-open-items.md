# Open items

1. Add proper near/far triangle clipping and a configurable camera up vector; add perspective edge-case fixtures.
2. Decide whether to implement lightweight mesh CSG, or make OpenCASCADE a separately evaluated optional backend if robust CAD Boolean operations become a requirement.
3. Decide whether a compressed PNG implementation is worth the added code/dependency cost. Stored DEFLATE currently keeps the binary self-contained and deterministic at the cost of larger files.
4. Use material roughness in shading or remove it from the next schema version until the renderer supports it. Define whether material handles should remain live references instead of being copied when a shape is created.
5. Consider OBJ material libraries and a documented glTF subset if interchange needs arise. STL currently omits appearance; OBJ has no MTL.
6. Keyframes and frame sequencing require the planned coordinated v4 loops and collection semantics. A later host can write numbered PNG frames; an ffmpeg-free codec/container has not been selected.
7. Add per-OS CI builds and record binary size, static-link result and shared-library audit for every published target. Only the current Linux ARM64 build has been measured.
8. Expand geometry and raster tests for clipping, transforms, visibility, materials, camera degeneracy, image limits, malformed paths and multiple overlapping surfaces.

The initial end-to-end still-image and STL path is working; these items are follow-on work, not blockers for the current milestone.
