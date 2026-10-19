# OCCT geometry integration

## Geometry path

All script-created box, sphere, cylinder, extruded sketch, and revolved profile shapes are OCCT `TopoDS_Shape` values. Shape operations (fuse, cut, common, fillet, chamfer, inward shell, and mirror) stay in BRep form. `BRepMesh_IncrementalMesh` tessellates at linear deflection 0.1 with relative mode off and serial execution; generated triangles and deterministic angle-gated vertex normals feed the existing software rasterizer and in-tree PNG/PPM/STL/OBJ writers. No OCCT exchange module is linked.

The base static closure from the spike is 14 OCCT archives. `TKOffset` is added as the 15th archive because `BRepOffsetAPI_MakeThickSolid` needs it for shelling. This is the smallest closure verified for the requested host operations; deleting `TKOffset` leaves unresolved shell symbols. The final aarch64 Release executable is fully static; the binary is 26,758,520 bytes (25.5 MiB), `file` identifies it as statically linked AArch64 ELF, and `ldd` reports `not a dynamic executable`; `03-build-run-test.md` has the build record.

`scene.fit_camera(fov)` frames the bounds of visible BReps. `Shape.volume()` and six `min_*`/`max_*` methods read BRep geometry, not its mesh. `hollow(t)` removes the lowest-Y planar face; `hollow_top(t)` removes the highest-Y planar face. `Sketch.point(x,y).close()` constructs a polygon, `scene.extrude(sketch,height,material)` extrudes along Z, and `scene.revolve_y(sketch,material)` revolves around Y. Geometry failures are converted to DSL host errors.

## Build123d volume cross-check

The Python calculations used the installed build123d/OCP packages through `/home/ubuntu/poc/venv/bin/python -I`; each one constructs the same dimensions and applies the same boolean/shell sequence as the `.lms` example. OCCT is queried by each script's `scene.assert_close(shape.volume(), expected, 1e-6)`. Results:

| Example | OCCT volume | build123d volume | Absolute difference |
| --- | ---: | ---: | ---: |
| plate with eight through-holes | 85.381416830866499 | 85.3814168308665 | < 1e-12 |
| six-by-two hollow brick with fused studs | 2422.398837915021 | 2422.39883791502 | < 1e-12 |
| polygon-profiled revolved vase with open top shell | 1735.8527040063793 | 1735.8527040063793 | < 1e-12 |

The plate and brick are practical, simpler constructions than their more detailed reference models: the plate uses a regular eight-hole grid; the brick uses a single inward shell and studs rather than the reference's internal rib lattice. The vase uses a polygonal profile rather than the reference's spline and arc profile, so its exact volume is cross-checked against the equivalent profile in build123d, not the unrelated reference profile's volume.

The rendered outputs were inspected against `reference/out/b3d_01_holes`, `b3d_02_lego`, and `b3d_03_vase`: the plate has clean through-holes; the brick has a readable two-by-six stud grid; the vase has a visibly open mouth and smooth rotational sides. Their STL files contain the same deterministic tessellation used by those PNGs. Example output paths are `build/plate-with-holes.png/.stl`, `build/brick.png/.stl`, and `build/vase.png/.stl`.

## DSL ergonomics note

A range-derived coordinate needs an explicit numeric conversion:

```lmscript
let x = -20.0 + float(ix) * 8.0;
```

Safe `int` to `float` widening in mixed arithmetic would make grid loops shorter. The examples retain explicit conversions; no compiler or SDK changes were made.

## Host geometry limits

Before meshing, the host restricts each BRep to a 1000-unit bounding-box span, absolute coordinates at most 1,000,000, 2000 face occurrences and 10,000 edge occurrences. Primitive dimensions and operation distances are 0.00001–1000; sketch coordinates are within ±500. Baked transforms receive the same envelope check before invoking OCCT. The meshing helper accepts deflection 0.01–1 (scripts use fixed 0.1); exported meshes are limited to 1,000,000 triangles. These host checks prevent extreme dimensions from driving absolute-deflection meshing and keep normal-key quantization in range; they are not VM budgets. Mesh volumes approximate BRep volumes and are not exact analytic volumes.
