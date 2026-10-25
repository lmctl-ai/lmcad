# lmcad examples

Every example is self-executing: run it from any empty directory and it writes its picture to `./images/` (meshes, frames and APNGs go to `./build/`). Hosts are bound with `cli=`/`install=`, so lmscript fetches the released `lmcad` (and `lmplot` where used) on first run.

## Stories (README gallery)

- `gear-train.lms`: 18/36-tooth involute spur gears, 36-frame seamless GIF; asserts the centre distance.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/gear-train.lms`
- `coil-spring.lms`: Free and loaded helical spring; k = Gd^4/(8D^3n) and the 17 mm deflection are asserted.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/coil-spring.lms`
- `pressure-vessel.lms`: Air receiver sized by ASME thin-wall formulas; 396 L capacity checked on the B-rep cavity.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/pressure-vessel.lms`
- `truss-bridge.lms`: Pratt truss, method of joints, members coloured and sized by force (lmcad + lmplot).  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/truss-bridge.lms`
- `beam-sections.lms`: Four equal-area beam sections, second moment of area and deflection (lmcad + lmplot).  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/beam-sections.lms`
- `plate-with-holes.lms`: Eight bores cut from an aluminium plate; volume, mass and removed volume asserted.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/plate-with-holes.lms`
- `heat-sink.lms`: Natural-convection fin-count sweep with the Elenbaas correlation (lmcad + lmplot).  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/heat-sink.lms`
- `dipole-field.lms`: Electric dipole field glyphs; far field checked against p/r^3.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/dipole-field.lms`
- `buckyball.lms`: C60 from truncated-icosahedron symmetry; 90 bonds, Euler V - E + F = 2, 7.1 A diameter.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/buckyball.lms`
- `mug.lms`: Revolved, shelled, filleted mug with a fused handle; capacity 629.5 mL from B-rep volumes.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/mug.lms`
- `vase.lms`: Bezier profile revolved and shelled; ceramic volume and 5.29 L capacity asserted.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/vase.lms`
- `brick.lms`: 2 x 6 brick, shelled with 12 revolved studs; volume and ABS mass asserted.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/brick.lms`

## Basics

- `box-volume.lms`: One box, a volume assertion and a 320 x 240 render.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/box-volume.lms`
- `box-stl.lms`: Binary STL export of one box (12 facets) to private implementation detail  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/box-stl.lms`
- `loop-row.lms`: A range loop places seven cubes.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/loop-row.lms`
- `still-life.lms`: Box, sphere and cylinder in one lit scene; also PPM and OBJ output in private implementation detail  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/still-life.lms`
- `section-view.lms`: Split a block with boolean intersections; the halves' volumes add up.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/section-view.lms`
- `ball-stick.lms`: Water molecule with tube_between bonds at the real 104.5 degree angle.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/ball-stick.lms`
- `tween.lms`: Sphere eased between two CSV keyframes: 30 PNG frames and an APNG in private implementation detail  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/tween.lms`
- `turntable.lms`: Camera orbit, 36-frame APNG converted to images/turntable.gif.  
  `lmctl autorun https://raw.githubusercontent.com/lmctl-ai/lmcad/main/examples/turntable.lms`

`render-all.sh` runs them all from the repository root and copies the gallery images into `docs/images/`; `../test.sh` runs every example against the locally built host.
