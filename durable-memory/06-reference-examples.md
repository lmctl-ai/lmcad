# 06 — Reference examples (build123d + pyvista)

What a C++ replacement for "Python CAD + render" has to cover, based on 12 real,
official examples that were run headlessly on this box (aarch64 Ubuntu, no display)
on 2026-10-18.

## Layout

- `reference/python/<name>.py`: the upstream example, **unmodified**, with a 4-line
  header (source URL, local path and commit, license). build123d checkout
  `092c40e3` (Apache-2.0); pyvista checkout `e9ae86b16` (MIT).
- `reference/runners/run_b3d.py`: runs a build123d example. `ocp_vscode` (the
  examples' viewer) is **not installed** in the venv, so the runner injects a
  stub that captures `show/show_object/show_all`, exports the captured shapes to
  one STL, and renders a PNG with pyvista off-screen.
- `reference/runners/run_pv.py`: runs a pyvista example with `pv.OFF_SCREEN=True`
  and turns each `Plotter.show()` into `<name>_<n>.png`. cwd = the output dir.
- `reference/out/<name>/`: outputs. Renders of downloaded third-party datasets (pv_03/04/05) are not committed; rerun the runner to regenerate them locally. All are < 1 MB except the 1.25 MB
  `barth_sextic.gif`, which is gitignored (`reference/.gitignore`).

Re-run (from a scratch dir, never from inside the repos):

```sh
R=<workspace>/lmcad/reference; cd /tmp/scratch
<workspace>/venv/bin/python -I $R/runners/run_b3d.py $R/python/b3d_05_tea_cup.py $R/out/b3d_05_tea_cup b3d_05_tea_cup
LMCAD_EXTRA_SITE=/tmp/lmcad_scratch/extra <workspace>/venv/bin/python -I $R/runners/run_pv.py $R/python/pv_06_marching_cubes.py $R/out/pv_06_marching_cubes pv_06_marching_cubes
```

## Environment findings

- **Headless pyvista works.** VTK 9.7.1 logs "bad X server connection" and then
  falls back to **EGL (Mesa)** off-screen by itself. No OSMesa or Xvfb is needed.
- `imageio` is **not** in the venv, so `Plotter.open_gif` fails. To avoid touching the
  shared venv, it was installed with `pip install --target /tmp/lmcad_scratch/extra imageio`.
  `run_pv.py` adds that dir to `sys.path` when `LMCAD_EXTRA_SITE` is set.
- `ocp_vscode` is not installed (see the stub above). Every build123d example
  imports it.
- pyvista `examples.download_*` datasets (bunny, carotid, office) were fetched from
  the network into pyvista's default cache (`~/.cache/pyvista`). That needs internet.
- build123d `Text` works. It uses system fonts via fontconfig (harmless warning).

## The 12 examples (simplest first within each library)

Rating = how hard to reproduce with a **small statically linked C++ software
rasterizer** that has box/sphere/cylinder primitives, transforms, a camera, one
light, and STL/OBJ/PNG output. 1 = trivial, 5 = needs a major subsystem.

### build123d

| # | name | what it draws | features | ran / output | rating (missing) |
|---|------|---------------|----------|--------------|------------------|
| 1 | `b3d_01_holes` (examples/holes.py) | 4 plates: through hole, counterbore, countersink (recessed + flush) | Box, Locations, Hole/CounterBoreHole/CounterSinkHole = boolean subtract of cylinder/cone | yes: `out/b3d_01_holes/b3d_01_holes.{stl,png}` (1848 tris) | **2**: mesh boolean subtract, cone primitive |
| 2 | `b3d_02_lego` (lego.py) | 6x2 Lego brick with pips and inner supports | BuildSketch Rectangle, 2D offset (walls), GridLocations arrays, extrude, Circle, Mode.SUBTRACT, ExportSVG (only behind a flag) | yes: `out/b3d_02_lego/b3d_02_lego.{stl,png}` (3980 tris) | **3**: 2D sketch + polygon offset, extrude, boolean, grid patterns |
| 3 | `b3d_03_vase` (vase.py) | revolved vase, shelled, top lip filleted | BuildLine (Line, RadiusArc, Spline, Polyline; `@`/`%` position/tangent), make_face, **revolve**, **shell (offset with opening)**, fillet selected by position/geom type | yes: `out/b3d_03_vase/b3d_03_vase.{stl,png}` (11422 tris) | **3**: splines/arcs → polyline, revolve (lathe), shell (= revolve a thickened profile); B-rep fillet is the hard part |
| 4 | `b3d_04_logo` (build123d_logo.py) | former build123d logo: text "123d"/"build" + dimension lines; extruded "3d" | **Text** (font outlines), lines/arrows, extrude, `ExportSVG` → `logo.svg` | yes: `out/b3d_04_logo/{b3d_04_logo.stl,b3d_04_logo.png,logo.svg}`. The STL/PNG contains only the extruded solid ("3d"); the full 2D logo is in logo.svg | **4**: font loading + glyph outline triangulation (stb_truetype-level), SVG export |
| 5 | `b3d_05_tea_cup` (tea_cup.py) | porcelain tea cup with swept handle | revolve, shell (offset openings), **fillet all edges**, **sweep** of a rounded rectangle along a spline, union, `export_gltf` | yes: `out/b3d_05_tea_cup/{b3d_05_tea_cup.stl,png, tea_cup.glb,.bin}` (12616 tris) | **5**: sweep along spline is doable; robust fillet of all edges + union is real B-rep/CSG |
| 6 | `b3d_06_joints` (joints.py) | tapered base box with 6 arms attached by Rigid/Revolute/Linear/Cylindrical/Ball joints | custom Solid subclass, extrude with **taper**, fillet, edge/face selectors (sort_by/filter_by), **Joints + connect_to** (assembly positioning), Location/Plane math | yes: `out/b3d_06_joints/b3d_06_joints.{stl,png}` (7 solids, 5858 tris) | **4**: assembly/joint frame math is easy; taper extrude + fillet + topology selectors need B-rep |

### pyvista

| # | name | what it draws | features | ran / output | rating (missing) |
|---|------|---------------|----------|--------------|------------------|
| 1 | `pv_01_geometric_objects` (00-load/create_geometric_objects.py) | 3x3 grid: cylinder, arrow, sphere, plane, line, box, cone, polygon, disc | primitive mesh generators, `subplot` grid, per-mesh colors | yes: `out/pv_01_geometric_objects/pv_01_geometric_objects_1.png` | **1**: needs cone/arrow/disc/polygon generators, multi-viewport |
| 2 | `pv_02_boolean_operations` (01-filter/boolean_operations.py) | sphere and cube union / difference / intersection, plus a sphere-cube difference | triangulate, **mesh booleans**, edges overlay, opacity | yes: 7 PNGs `out/pv_02_boolean_operations/*_1..7.png` | **3**: robust triangle-mesh CSG |
| 3 | `pv_03_clip_with_plane_box` (01-filter/clip_with_plane_box.py) | bunny clipped by a plane, office clipped by a box, airplane clip_surface, wavelet clip_scalar | **clip by plane / box / surface / scalar**, downloaded datasets, camera positions, subplots | ran (5 PNGs); outputs not committed: derived from downloaded datasets (bunny, office, airplane) with unclear redistribution terms | **2**: plane clip of triangles is easy; box/surface clip are moderate |
| 4 | `pv_04_light_types` (04-lights/light_types.py) | Stanford bunny lit by a headlight, a camera light, a scene light | **Light types and positions**, smooth shading, OBJ/PLY loading | ran (3 PNGs); outputs not committed: renders of the Stanford bunny dataset | **2**: >1 light type, vertex normals / smooth shading, mesh import |
| 5 | `pv_05_glyph` (01-filter/glyph.py) | carotid artery velocity arrows; sphere normals as arrows; random-hills glyphs | threshold, **glyphs** (oriented and scaled instances), **scalar → colormap**, scalar bar, vectors | ran (4 PNGs); outputs not committed: derived from the downloaded carotid dataset | **3**: instancing, per-vertex scalars + colormap LUT, scalar-bar text |
| 6 | `pv_06_marching_cubes` (01-filter/marching_cubes.py) | Barth sextic implicit surface; marching cubes vs flying edges; animated isovalue sweep | ImageData scalar field, **contour (marching cubes / flying edges)**, **open_gif / write_frame** animation | yes (with imageio from scratch dir): `out/pv_06_marching_cubes/pv_06_marching_cubes_{1,2}.png` + `barth_sextic.gif` (1.25 MB, gitignored) | **3**: marching cubes (~300 lines + tables), GIF encoder (or PNG frame sequence) |

Other well-known examples that were considered but not run: build123d `bicycle_tire`,
`cast_bearing_unit`, `heat_exchanger`, `toy_truck`, `stud_wall`, `platonic_solids`
(needs scipy ConvexHull); pyvista `02-plot/orbit.py`, `gif.py`, `colormap.py`,
`surface_reconstruction.py`, `04-lights/shadows.py`, `bolt-nut` (in pyvista doc).

## Capabilities a C++ replacement needs, most valuable first

1. **Mesh primitives beyond box/sphere/cylinder**: cone, torus, disc, arrow, plane, polygon
   with resolution control. Every example uses them.
2. **Triangle-mesh booleans (union/difference/intersection)**: holes, lego, joints, tea
   cup, pv booleans. The single biggest CAD unlock.
3. **2D sketch → solid**: polylines, arcs, splines (Bezier/Catmull-Rom), polygon offset,
   then **extrude (with taper)** and **revolve**. Covers lego, vase, tea-cup body.
4. **Smooth shading + per-vertex normals, several lights (headlight/camera/scene),
   multi-viewport subplots**: covers most of the pyvista look.
5. **Per-vertex scalars → colormap LUT + scalar bar**: glyph, clip_scalar, contouring.
6. **Patterns/instancing (Locations, GridLocations, glyphs)**: cheap and widely used.
7. **Sweep along a path and shell/thicken**: tea cup handle, vase walls.
8. **Clipping (plane/box) and isosurfacing (marching cubes on a grid)**.
9. **Animation output**: frame sequence → GIF (or APNG) with a capped size.
10. **Mesh import (STL/OBJ/PLY) and export (STL/OBJ/glTF/SVG)**.
11. **Text (font outlines → extruded solids)**: needs an embedded TTF + rasterizer.
12. **Fillet/chamfer**: true B-rep fillets are out of reach. Approximate them only for
    simple cases (rounded box, filleted revolve profile done in 2D before revolving).
