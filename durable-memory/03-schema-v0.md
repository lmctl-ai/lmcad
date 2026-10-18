# Schema v0 proposal

This is a compact schema sketch in the same `dsl.schema: 1` JSON form as `lmor-dsl/schema/*.dsl.json`. It aims for one useful still image while keeping geometry ownership in the host. The method count is 20, plus one function. The schema is a compiler contract; the implementation must bind every declared signature in the VM registry. Validate the exact accepted schema type names against the current lmscript release before treating this proposal as compiler-ready.

```json
{
  "dsl.schema": 1,
  "name": "lmcad",
  "types": {
    "Scene": { "methods": {
      "box": { "params": ["float", "float", "float", "Material"], "returns": "Shape" },
      "sphere": { "params": ["float", "Material"], "returns": "Shape" },
      "cylinder": { "params": ["float", "float", "Material"], "returns": "Shape" },
      "camera": { "params": ["float", "float", "float", "float", "float", "float", "float"], "returns": "Camera" },
      "directional_light": { "params": ["float", "float", "float", "float"], "returns": "Light" },
      "background": { "params": ["float", "float", "float"], "returns": "Scene" },
      "render_png": { "params": ["string", "int", "int"], "returns": "void" },
      "render_ppm": { "params": ["string", "int", "int"], "returns": "void" },
      "export_stl": { "params": ["string"], "returns": "void" },
      "export_obj": { "params": ["string"], "returns": "void" }
    } },
    "Shape": { "methods": {
      "translate": { "params": ["float", "float", "float"], "returns": "Shape" },
      "rotate": { "params": ["float", "float", "float"], "returns": "Shape" },
      "scale": { "params": ["float", "float", "float"], "returns": "Shape" },
      "visible": { "params": ["bool"], "returns": "Shape" },
      "name": { "params": ["string"], "returns": "Shape" }
    } },
    "Camera": { "methods": {
      "look_at": { "params": ["float", "float", "float", "float", "float", "float"], "returns": "Camera" },
      "clipping": { "params": ["float", "float"], "returns": "Camera" }
    } },
    "Material": { "methods": {
      "color": { "params": ["float", "float", "float"], "returns": "Material" },
      "roughness": { "params": ["float"], "returns": "Material" }
    } },
    "Light": { "methods": {
      "intensity": { "params": ["float"], "returns": "Light" }
    } }
  },
  "functions": {
    "material": { "params": [], "returns": "Material" }
  },
  "context": { "scene": "Scene" }
}
```

## Signature notes

- `box(x, y, z, material)`, `sphere(radius, material)`, and `cylinder(radius, height, material)` append a primitive to the scene and return a stable shape handle. Dimensions must be positive and finite.
- Shape transforms mutate that handle's transform and return it to permit chaining. Rotation parameters are Euler angles in radians, in documented XYZ order. Scale components must be finite and nonzero; whether negative scale is supported should be decided before implementation.
- `material()` creates a default material. `color(r,g,b)` clamps or rejects values outside `[0,1]` (prefer rejection with a clear host error); `roughness` is reserved for future shading and may be omitted from the first renderer if the signature is fixed.
- `scene.camera(fov_degrees, eye_x, eye_y, eye_z, target_x, target_y, target_z)` constructs the active perspective camera; `look_at` updates target and eye values on the handle. Document vertical FOV and near/far clipping defaults at implementation time. The seven scalar arguments keep the schema within primitive parameter types.
- `directional_light(x,y,z,intensity)` creates the initial single light direction and intensity; `.intensity` adjusts it. Normalize direction in the host and reject a zero vector.
- `background(r,g,b)` sets the scene's background color in `[0,1]` and returns the scene. `render_png(path,width,height)` renders the scene to one PNG. Bound dimensions and total pixels in the host before allocation. Require a camera.
- `export_stl(path)` exports all visible scene triangles in world space as binary STL. STL carries no material information.
- `Material` is a host object returned by the free `material()` constructor. A later schema revision may add a default material argument strategy if lmscript supports it; v0 uses explicit materials.

This surface has 20 methods across the five types (10 Scene, 5 Shape, 2 Camera, 2 Material, 1 Light), plus one free function. The first implementation can keep shading simple; every declared signature must still be registered before the schema is used to compile runnable chunks.

## Context and lifetime

The host creates one `Scene` object and supplies it as context `scene`. Scene-owned vectors/arenas retain every returned handle until VM execution and render/export complete. A handle must not outlive its scene. A script that calls `render_png` more than once may render the same scene at multiple sizes or paths; the host should specify whether later calls replace or accumulate output (recommended: each call independently writes the current scene). A run-time error aborts the chunk and maps to the host's documented DSL error exit code.

Before freezing v0, confirm lmscript's accepted built-in type spellings (`string` versus `str`) and whether chained method return types are supported; mirror the verified spellings in a compiler fixture and schema tests in a later implementation step.
