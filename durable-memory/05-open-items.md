# Open items

1. Add a configurable camera up vector and more perspective edge-case tests.
2. Decide whether material roughness should affect shading or be removed from a future schema revision.
3. Consider OBJ MTL support or a documented glTF subset if interchange needs grow.
4. APNG is the current ffmpeg-free animation output. Delta-frame encoding could reduce file sizes; full-canvas frames currently
   stream with deterministic compression. Host validation limits animations to 240 frames and 64 million cumulative pixels.
5. Expand raster tests for clipping, overlapping surfaces, and camera edge cases.
6. The VM retains defensive checks for malformed runtime values and missing host context objects. It has no instruction budget or
   stack, local, or string-size caps. Host validation remains separate from VM behavior.
7. A script must explicitly convert loop integers to floats in mixed arithmetic, for example `float(ix) * 8.0`. Implicit safe
   int-to-float widening would make grid arithmetic shorter; current examples retain explicit conversions.
8. The section example separates two intersected solids rather than rendering a general section plane. A half-space primitive or
   cut-face material override would support arbitrary section views more directly.
9. The mug handle is a polygonal extruded loop and its vessel uses a polygonal revolved profile. Arc/sweep sketches would make
   both contours smoother.
10. Gallery examples (2026-10-23) hit these gaps, all logged in simulation/DRAW_NEEDS.md: no cone primitive (arrow heads),
    no helix/path sweep (coil spring = ~550 tubes), no Shape.area(), no cut-face colour / section render, and the DSL has no
    atan2/exp/log. Switch coil-spring, heat-sink and pressure-vessel to the new API once a release provides it.
11. CI runs the lmplot combo examples only where GITHUB_TOKEN lets lmscript install lmplot (skipped on Windows); GIF output needs
    ffmpeg, which the examples treat as optional.
