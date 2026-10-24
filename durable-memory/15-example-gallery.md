# 15 — Example gallery (stories, known answers, how the pictures are made)

All examples self-execute (see 03-build-run-test.md). README gallery = the 12 stories below, in four themed tables, plus a fifth table of scenes shared with lmblender/lmmagnum/lmfilament (the last five rows),
each caption = one line + `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/<name>.lms`.
Images in `docs/images/` come from `examples/render-all.sh` (never edit them by hand).

| example | hosts | known answer asserted on the host | output |
| --- | --- | --- | --- |
| gear-train | lmcad + ffmpeg | centre distance m(z1+z2)/2 = 54 mm; involute flanks from the base-circle parametric form (no atan in DSL), phase pi/z2 for the even gear | gear-train.gif 720x420 (36 frames, seamless) + png |
| coil-spring | lmcad | k = G d^4/(8 D^3 n) = 11.748 N/mm, 200 N -> 17.02 mm | coil-spring.png |
| pressure-vessel | lmcad | t_shell = pR/(S-0.6p) = 6.61 mm, t_head = pR/(2S-0.2p) = 3.27 mm, cavity B-rep volume = 395.84 L within 0.2 % | pressure-vessel.png (cm units) |
| truss-bridge | lmcad + lmplot | Pratt truss method of joints, R = 250 kN, mid top chord -450 kN, centre vertical 0 | truss-bridge.png 900x760 |
| beam-sections | lmcad + lmplot | four sections of equal area (B-rep volume = A*L each); I-beam 25x stiffer than solid square | beam-sections.png 900x700 |
| plate-with-holes | lmcad | plate volume 85.38 cm^3, removed = 8*pi*0.65^2, 230.5 g of 6061 | plate-with-holes.png |
| heat-sink | lmcad + lmplot | Elenbaas parallel-plate sweep, best N = 8 fins (S = 6.9 mm, 4.7 K/W); Bar-Cohen S_opt = 5.9 mm | heat-sink.png 900x540 |
| dipole-field | lmcad | exact two-charge field on the bisector at r = 20 matches 2qd/r^3 within 0.4 % | dipole-field.png |
| buckyball | lmcad | truncated icosahedron: 90 bonds, V - E + F = 2, diameter 7.1 A at 1.44 A bonds | buckyball.png |
| mug | lmcad | capacity (volume removed by hollow_top) 629.53 mL, within 0.5 % of a frustum hand check | mug.png (STL exported before the coffee disc) |
| vase | lmcad | ceramic 1708.27 cm^3, capacity 5287.73 cm^3 | vase.png |
| brick | lmcad | volume 2548.86 mm^3 -> 2.68 g ABS | brick.png |
| caffeine-molecule | lmcad | none (coordinates copied from lmblender) | caffeine-molecule.png 900x600 |
| dna-double-helix | lmcad | none; 28 base pairs, 34.3 deg twist, 0.338 nm rise (from lmblender) | dna-double-helix.png 900x500 |
| cornell-box | lmcad | none; boxes + lamp, no global illumination (from lmblender) | cornell-box.png 640x640 |
| saturn-rings | lmcad | none; ring bands are cylinders with `cut` bores, radii from lmblender | saturn-rings.png 900x560 |
| pbr-spheres, material-sandbox, model-viewer, primitives | lmcad | none; ported from lmmagnum/lmfilament | one PNG each |
| multidraw, cubemap, bullet, fluidsimulation3d, fluidsimulation2d, textured-triangle | lmcad | none; magnum-examples analogues (bullet = closed-form ballistics, fluids = streamlines of analytic flows, no solver; textured-triangle extrudes 64 sketch triangles) | one PNG each |

Basics (examples/README.md only): box-volume, box-stl, loop-row, still-life, section-view, ball-stick (water at 104.5 deg),
tween (writes build/keyframes.csv itself), turntable (APNG -> images/turntable.gif).

Patterns worth reusing:
- lmcad PPM into lmplot: `scene.render_ppm("build/x.ppm", W, H)` then in lmplot
  `plot 'build/x.ppm' binary array=(W,H) skip=15 flipy format='%uchar' using 1:2:3 with rgbimage` with `set size ratio -1`,
  `unset tics; unset border` (header "P6\nW H\n255\n" is 15 bytes for 3-digit W and H). Re-enable `set xtics; set ytics` for the
  chart panel after `unset tics`.
- Light backgrounds (0.96-1.0) and explicit cameras frame better than `fit_camera` for wide/flat parts (fit_camera uses the
  bounding sphere from a fixed (1.4,1.1,1.7) direction).
- `Shape.rotate` is in radians and accumulates; booleans bake the transform and reset it.
- Color legends, labels and numbers need a second host; lmcad has no text.

Missing lmcad features met (logged in simulation/DRAW_NEEDS.md): cone/arrow-head primitive (coil-spring load arrow, truss loads),
helix sweep (spring built from ~550 tubes and spheres), `atan2` in the DSL (involute polar angle), shape surface area (heat-sink
fin area), per-face colour for cut faces (pressure-vessel cutaway uses a separate inner-coat solid).
