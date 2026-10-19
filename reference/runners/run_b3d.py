"""Run an unmodified build123d example headlessly.

ocp_vscode (the examples' viewer) is not installed here, so a stub module is
injected that captures show/show_object/show_all arguments. The captured shapes
are exported as one STL and rendered to a PNG with pyvista off-screen.
Usage: python -I run_b3d.py <example.py> <out_dir> <name>
"""
import inspect, os, runpy, sys, types

import build123d as bd
import pyvista as pv

script, out_dir, name = sys.argv[1:4]
captured = []


def _add(obj):
    if isinstance(obj, (list, tuple)):
        for o in obj:
            _add(o)
        return
    if isinstance(obj, (bd.BuildPart, bd.BuildSketch, bd.BuildLine)):
        obj = obj._obj
    if obj is None:
        return
    if not isinstance(obj, bd.Shape):  # raw TopoDS_Shape from `.wrapped`
        try:
            obj = bd.Compound(obj)
        except Exception:
            return
    captured.append(obj)


def show(*objs, **_kw):
    for o in objs:
        _add(o)


def show_all(*_a, **_kw):
    g = inspect.currentframe().f_back.f_globals
    for k, v in list(g.items()):
        if not k.startswith("_") and isinstance(v, (bd.Shape, bd.BuildPart)):
            _add(v)


stub = types.ModuleType("ocp_vscode")
stub.show = show
stub.show_object = show
stub.show_all = show_all
stub.__all__ = ["show", "show_object", "show_all", "set_port", "set_defaults", "Camera"]
stub.__getattr__ = lambda attr: (lambda *a, **k: None)
for extra in ("set_port", "set_defaults", "Camera", "reset_show"):
    setattr(stub, extra, lambda *a, **k: None)
sys.modules["ocp_vscode"] = stub

os.makedirs(out_dir, exist_ok=True)
os.chdir(out_dir)
runpy.run_path(script, run_name="__main__")

solids = [s for s in captured if s.volume > 1e-9]
faces = [s for s in captured if s.volume <= 1e-9 and s.faces()]
shown = solids or faces
print(f"captured {len(captured)} objects, {len(solids)} with volume")
comp = bd.Compound(children=shown)
stl = f"{name}.stl"
bd.export_stl(comp, stl, tolerance=0.05, angular_tolerance=0.3)
mesh = pv.read(stl)
pl = pv.Plotter(off_screen=True, window_size=(640, 480))
pl.add_mesh(mesh, color="lightsteelblue", smooth_shading=True, split_sharp_edges=True)
pl.view_isometric()
pl.screenshot(f"{name}.png")
print(f"wrote {stl} ({mesh.n_cells} tris) and {name}.png")
