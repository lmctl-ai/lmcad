"""Run an unmodified pyvista gallery example headlessly.

Forces off-screen rendering and turns every Plotter.show() into a PNG
screenshot <name>_<n>.png in out_dir (cwd is out_dir, so GIFs land there too).
Usage: python -I run_pv.py <example.py> <out_dir> <name>
"""
import os, runpy, sys

import pyvista as pv

# Optional extra site dir (e.g. imageio installed with pip --target) without touching the venv.
if os.environ.get("LMCAD_EXTRA_SITE"):
    sys.path.append(os.environ["LMCAD_EXTRA_SITE"])

script, out_dir, name = sys.argv[1:4]
pv.OFF_SCREEN = True
counter = [0]
_orig_show = pv.Plotter.show


def show(self, *args, **kwargs):
    counter[0] += 1
    kwargs.pop("screenshot", None)
    kwargs["auto_close"] = False
    cpos = _orig_show(self, *args, screenshot=f"{name}_{counter[0]}.png", **kwargs)
    self.close()
    return cpos


pv.Plotter.show = show
os.makedirs(out_dir, exist_ok=True)
os.chdir(out_dir)
runpy.run_path(script, run_name="__main__")
print(f"{name}: {counter[0]} screenshots")
