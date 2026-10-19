from pathlib import Path
import json
from build123d import Align, Axis, Box, Cylinder, Polygon, Pos, Plane, extrude, fillet, revolve, export_stl

out = Path(__file__).parent / "out-py"
out.mkdir(exist_ok=True)

def report(name, shape):
    triangles = shape.tessellate(0.1, 0.5)[1]
    export_stl(shape, out / f"{name}.stl", tolerance=0.1, angular_tolerance=0.5)
    bounds = shape.bounding_box()
    values = {"volume": shape.volume, "area": shape.area,
              "bbox": [bounds.min.X, bounds.min.Y, bounds.min.Z,
                       bounds.max.X, bounds.max.Y, bounds.max.Z],
              "triangles": len(triangles)}
    print(name, json.dumps(values, sort_keys=True))

box = Box(40, 30, 20, align=(Align.MIN, Align.MIN, Align.MIN))
vertical = [edge for edge in box.edges()
            if abs(edge.tangent_at(0.5).Z) > 0.999999]
rounded = fillet(vertical, 3)
cutter = Pos(20, 15, -1) * Cylinder(
    6, 22, align=(Align.CENTER, Align.CENTER, Align.MIN))
report("box_fillet_hole", rounded - cutter)

sketch = Polygon((0, 0), (25, 0), (20, 18), (5, 22), align=None)
report("polygon_extrusion", extrude(sketch, amount=10))

profile = Plane.XZ * Polygon((10, 0), (15, 0), (15, 20), (10, 20), align=None)
report("profile_revolution", revolve(profile, axis=Axis.Z))
