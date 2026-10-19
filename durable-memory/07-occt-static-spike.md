# OCCT static headless spike

## Result

Direct OCCT host calls work in a headless, fully static C++ executable on this aarch64 Linux host. The measured executable has no runtime shared-library dependencies, is 24,447,304 bytes (23.3 MiB), and renders three analytic test shapes to binary STL in 0.01 seconds. This is technically feasible for lmcad, but adds a sizeable native CAD kernel and a source build of about 16 minutes on the four-core test machine. It is not a dependency-free source tree: OCCT must be built and redistributed under its license terms.

Recommendation: keep the current small software rasterizer for the first image path. Adopt OCCT when CAD-kernel operations such as robust booleans, fillets, shelling, or STEP become a product requirement. If adopting it, use the minimal static toolkit closure measured below, build the kernel in a per-OS CI cache, and use a small in-tree STL writer unless the larger OCCT exchange stack is wanted.

## Version and license

The installed `cadquery-ocp-novtk` wheel reports distribution version `8.0.1.1.0`; its binary strings identify `occt-8.0.1-novtk`. The matching shallow source checkout is OCCT tag `V8_0_1`, commit `b8f597c677811d1f9f4d8a97f5ae2825c0353a42`. The wheel's `Standard_Version` Python import was unavailable, so the version was cross-checked from package metadata and wheel strings. `build123d` installed in the PoC venv was `0.13.1.dev229+g092c40e38`.

OCCT is LGPL-2.1 with the Open CASCADE exception. OCCT's own CMake configure warns about limitations on static linking with proprietary software and points to commercial licensing exceptions. Review the license and redistribution obligations before shipping a statically linked product. Primary references: [OCCT releases](https://github.com/Open-Cascade-SAS/OCCT/releases), [OCCT license text](https://github.com/Open-Cascade-SAS/OCCT/blob/master/OCCT_LGPL_EXCEPTION.txt).

## Host and minimal geometry build

Host: aarch64 Linux, GCC/G++ 13.3, CMake 3.28.3, Ninja, 4 cores, 15 GiB RAM. Build used `nice -n 10`, at most three jobs, Release, static libraries, no PIC, and disabled Tcl, FreeType, TBB, X11, OpenGL, Draw, Visualization, ApplicationFramework, and DataExchange. Configure succeeded. The actual additional-toolkit request was `TKPrim;TKBO;TKFillet;TKOffset;TKMesh`; OCCT resolved the dependencies to these 15 static archives:

| Archive | Bytes |
| --- | ---: |
| TKernel | 4,835,446 |
| TKMath | 5,277,320 |
| TKG2d | 1,627,834 |
| TKG3d | 5,184,896 |
| TKGeomBase | 15,202,388 |
| TKBRep | 7,759,508 |
| TKGeomAlgo | 18,856,172 |
| TKTopAlgo | 12,426,158 |
| TKPrim | 1,420,106 |
| TKBO | 12,670,946 |
| TKShHealing | 11,662,616 |
| TKFillet | 8,272,738 |
| TKBool | 18,228,478 |
| TKOffset | 6,055,866 |
| TKMesh | 4,729,094 |

The build completed successfully in **15:51.52 wall time**, with **395,208 KiB maximum RSS** reported by `/usr/bin/time -v`. The generated build directory was 273 MiB. Only 14 archives are needed by this spike; excluding unused `TKOffset`, their total archive size is **128,153,700 bytes (122.2 MiB)**. The program is linked against the exact 14 archives in `spikes/occt/spike.cpp`'s build command below; `--start-group/--end-group` resolves OCCT's cyclic static references.

## Geometry, binary, and comparison

The C++ spike creates a 40x30x20 box, fillets its four vertical edges at radius 3, cuts a through-hole of radius 6, extrudes a four-sided sketch, and revolves a rectangular profile. It tessellates at linear deflection 0.1, relative mode enabled, angular deflection 0.5, matching build123d's `Shape.mesh()` call. It reports volume, area, precise axis-aligned bounds, and triangle count, and writes binary STL with a small in-tree writer. This intentionally avoids OCCT's stock STL exchange toolkit in the minimal link measurement.

| Shape | Volume OCCT / build123d | Area OCCT / build123d | Bounds | Triangles OCCT / build123d |
| --- | --- | --- | --- | ---: |
| Filleted box with hole | 21583.5399671 / 21583.5399671 | 5609.327352 / 5609.327351998 | (0,0,0)–(40,30,20) | 232 / 232 |
| Polygon extrusion | 4000 / 3999.9999999999995 | 1617.66744734 / 1617.667447339 | (0,0,0)–(25,22,10) | 12 / 12 |
| Profile revolution | 7853.98163397 / 7853.981633974 | 3926.99081699 / 3926.990816987 | (-15,-15,0)–(15,15,20) | 208 / 208 |

Volumes and areas agree to floating-point rounding; bounds and tessellation counts match exactly. The first comparison used absolute deflection and did not match build123d's relative-mode tessellation; enabling relative mode produced identical counts. STL output counts were read only after closing the writer to flush its header and facets. A run of all three shapes took **0.01 s**, with 15,140 KiB max RSS. The full-static executable is **24,447,304 bytes**; `file` identifies it as statically linked AArch64 ELF and `ldd` reports `not a dynamic executable`. A second build with `-static-libstdc++ -static-libgcc` but dynamic glibc is **24,294,144 bytes**; `ldd` lists only `libm.so.6`, `libc.so.6`, and the loader.

## Exchange toolkit cost

The OCCT 8.0.1 STL writer toolkit is named `TKDESTL` (not `TKSTL`). Its direct dependencies include `TKDE`, `TKLCAF`, and `TKXCAF`; the configured Ninja graph for `TKDESTL` had **2,680 tasks** and pulled additional service/viewer-related libraries (`TKService`, `TKV3d`, `TKVCAF`, among others). It was not built for this spike. The STEP writer is `TKDESTEP`; a separate static configure succeeded, but its Ninja graph had **4,216 tasks**. It depends on `TKXSBase`, CAF/CDF/LCAF/XCAF, geometry and healing toolkits. STEP was not built; this closure is substantially larger than the geometry-only build and was not considered cheap for the first milestone.

## Reproduction

All build outputs and the shallow OCCT checkout stayed in `<workspace>/occt-spike`, outside the repos. From that directory:

```sh
git clone --depth 1 --branch V8_0_1 https://github.com/Open-Cascade-SAS/OCCT.git occt-src
nice -n 10 cmake -S occt-src -B build-geom -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_LIBRARY_TYPE=Static \
  -DBUILD_MODULE_ApplicationFramework=OFF -DBUILD_MODULE_DataExchange=OFF \
  -DBUILD_MODULE_Draw=OFF -DBUILD_MODULE_FoundationClasses=OFF \
  -DBUILD_MODULE_ModelingAlgorithms=OFF -DBUILD_MODULE_ModelingData=OFF \
  -DBUILD_MODULE_Visualization=OFF \
  '-DBUILD_ADDITIONAL_TOOLKITS=TKPrim;TKBO;TKFillet;TKOffset;TKMesh' \
  -DBUILD_DOC_Overview=OFF -DBUILD_DOC_RefMan=OFF -DUSE_TK=OFF \
  -DUSE_FREETYPE=OFF -DUSE_TBB=OFF -DUSE_XLIB=OFF -DUSE_OPENGL=OFF \
  -DBUILD_USE_PCH=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=OFF
/usr/bin/time -v nice -n 10 cmake --build build-geom --parallel 3
mkdir -p out-cpp
nice -n 10 g++ -std=c++17 -O2 -Ibuild-geom/include/opencascade \
  <workspace>/lmcad/spikes/occt/spike.cpp -static \
  -Wl,--start-group \
  build-geom/lin64/gcc/lib/libTKernel.a \
  build-geom/lin64/gcc/lib/libTKMath.a \
  build-geom/lin64/gcc/lib/libTKG2d.a \
  build-geom/lin64/gcc/lib/libTKG3d.a \
  build-geom/lin64/gcc/lib/libTKGeomBase.a \
  build-geom/lin64/gcc/lib/libTKBRep.a \
  build-geom/lin64/gcc/lib/libTKGeomAlgo.a \
  build-geom/lin64/gcc/lib/libTKTopAlgo.a \
  build-geom/lin64/gcc/lib/libTKShHealing.a \
  build-geom/lin64/gcc/lib/libTKPrim.a \
  build-geom/lin64/gcc/lib/libTKBO.a \
  build-geom/lin64/gcc/lib/libTKBool.a \
  build-geom/lin64/gcc/lib/libTKFillet.a \
  build-geom/lin64/gcc/lib/libTKMesh.a \
  -Wl,--end-group -pthread -ldl -lm -lrt -o spike
mkdir -p out-cpp
./spike out-cpp
ldd spike
<workspace>/venv/bin/python -I \
  <workspace>/lmcad/spikes/occt/cross_build123d.py
```

The C++ and Python source files are in `spikes/occt/`. Configure-only exchange measurements used the same flags with `-DBUILD_ADDITIONAL_TOOLKITS=TKDESTL` and `TKDESTEP`, then `ninja -n`.

## CI cost estimate

Observed cost on the available 4-core aarch64 box is 15m52 per clean geometry build, before application link. For three independent OS runners, budget roughly **45–90 runner-minutes total** (about 15–30 minutes each, with Windows likely slower); these jobs can run concurrently. The 273 MiB build tree and 122.2 MiB linked archive closure are practical cache artifacts. Cache keys should include OCCT tag, OS, architecture, compiler/toolchain, and build options. Cache misses or compiler upgrades pay the source-build cost again. STEP's 4,216-task graph should be budgeted separately and will increase both cold build time and cache size.
