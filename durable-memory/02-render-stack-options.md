# Render stack options

## Recommendation

For milestone one, use a small project-owned triangle mesh layer and CPU software rasterizer, with a tiny self-contained PNG writer. Keep it in ordinary C++17 and the standard library. This is the shortest route to the acceptance path `lmscript -> .dslb -> embedded VM -> host scene -> PNG`, and it avoids introducing a platform graphics runtime or a large third-party build before the schema and host boundary are proven.

This choice targets predictable headless output and a statically linked CLI. It is not a proposal to build a general CAD kernel. Implement a camera projection, depth buffer, flat or simple Lambert shading, and triangle rasterization first. Provide PPM as a trivially inspectable debug output; write PNG using a compact in-tree encoder that emits PNG chunks and zlib stored DEFLATE blocks. This avoids a runtime dependency on libpng or zlib. Benchmark and replace the encoder only if output size or render time becomes a demonstrated problem.

## Options compared

| Option | Fit | Static-build and delivery considerations | Decision |
| --- | --- | --- | --- |
| Own mesh primitives + CPU rasterizer | Directly covers box, sphere, cylinder, transforms, camera, light, raster output and STL/OBJ. A useful first implementation can be a few focused source files. | No external libraries; static link is just the C++ runtime/toolchain choice. PNG encoding is small enough to own. Work and risk are in correct geometry, clipping, depth and rasterization rather than build integration. | **Choose for first milestone.** Add mesh export after the PNG path. Treat CSG as a later, explicitly limited feature (for example, mesh Boolean via a selected algorithm), not as part of v0. |
| OpenCASCADE static | Strong B-rep modeling, Boolean operations and CAD exchange formats. Appropriate if exact CAD solids and robust CSG become product requirements. | Not installed here. A source build and static closure are a substantial project; packaging all transitive libraries and licensing notices also needs deliberate work. The final binary and build time would be far beyond what the PNG proof needs. Planning order of magnitude: many minutes to hours for a clean build and tens to hundreds of MB of linked artifacts, depending on modules and static dependencies; not measured on this box. | Defer until exact solids or CAD interoperability justify it. |
| VTK offscreen | Broad visualization, rendering and scientific-data ecosystem; can render without a visible window when configured with a suitable backend. | Not installed here. Static VTK means selecting and building the module closure and its rendering dependencies; headless backend behavior and per-OS packaging add complexity. Planning order of magnitude: tens of minutes to hours and a large dependency/binary footprint (typically hundreds of MB of build inputs/artifacts); not measured on this box. | Defer; too much framework for a first small still-image path. |

The estimates for OpenCASCADE and VTK are planning ranges, not measurements or vendor guarantees. No static builds were attempted. If either is reconsidered, first build a minimal static sample on each target OS and measure configure/build time, linked size, and runtime shared-library dependencies.

## Machine checked (2026-10-18)

- Linux `aarch64`, AWS Neoverse-V1, 4 online CPUs.
- 15 GiB RAM, about 13 GiB available at check time; no swap.
- `g++` 13.3.0, CMake 3.28.3, Ninja 1.11.1.
- `pkg-config` reports libpng 1.6.43 and `libpng-dev` is installed. No OpenCASCADE or VTK package, shared library, or pkg-config entry was found. The selected first milestone does not need the installed libpng and should not depend on it for a self-contained binary.

## Proposed render and export increments

1. **PNG vertical slice:** triangle mesh, box/sphere/cylinder primitive generation, translation/rotation/scale, camera, one directional light, material color, depth-tested software rasterizer, PPM debug and PNG output. Set bounded image dimensions and validate camera parameters in the host.
2. **Mesh interchange:** STL and OBJ writers over the same triangle mesh. Add glTF only with a documented small subset and a test fixture; it introduces JSON/buffer packaging choices that are unnecessary for milestone one.
3. **Animation output:** deterministic numbered PNG frames. A later wrapper may pipe frames to an externally installed ffmpeg; lmcad itself need not depend on or embed ffmpeg. If a self-contained video container is required, decide separately after frame generation is stable.
4. **CSG-light:** defer and define the supported operations and failure behavior first. Mesh Boolean robustness is not a free extension of primitive creation.

The initial binary should be checked with `ldd` (or the target platform equivalent) to confirm that no non-system shared rendering library is required. A fully static C++ runtime may be platform-specific; make the supported delivery claim per target OS after CI builds exist.
