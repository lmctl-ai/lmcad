# Relinking a Linux lmcad binary with modified OCCT

Each Linux binary artifact ships a matching `lmcad-linux-<arch>-relink-objects.tar.gz`
containing the application's CMake-built object files for that architecture. These
objects can be linked against a modified OCCT static install without rebuilding
the lmcad sources. The archive requires the matching architecture, GNU toolchain
ABI, and the static library closure below.

## Rebuild OCCT

Obtain OCCT source tag `V8_0_1`, or a modified source tree, then build/install its
static archives using the shipped `scripts/build-occt.sh` and
`.github/occt-cmake-flags.txt`. The script accepts `OCCT_SRC`, `OCCT_BUILD`, and
`OCCT_PREFIX` overrides. Preserve the existing library names and install layout.

## Link the supplied objects

Extract the architecture-specific object archive and modified OCCT prefix. With
GCC on Linux, use the matching toolchain (CI currently records GCC 13.3.0) and
run the equivalent of this command, replacing paths with the extracted locations:

```sh
g++ -static -static-libstdc++ -static-libgcc \
  CMakeFiles/lmcad.dir/cpp/main.cpp.o \
  CMakeFiles/lmcad.dir/cpp/cad.cpp.o \
  CMakeFiles/lmcad.dir/cpp/scene.cpp.o \
  CMakeFiles/lmcad.dir/cpp/render.cpp.o \
  CMakeFiles/lmcad.dir/cpp/image.cpp.o \
  CMakeFiles/lmcad.dir/cpp/export.cpp.o \
  -Wl,--start-group \
  "$OCCT_PREFIX/lib/libTKernel.a" "$OCCT_PREFIX/lib/libTKMath.a" \
  "$OCCT_PREFIX/lib/libTKG2d.a" "$OCCT_PREFIX/lib/libTKG3d.a" \
  "$OCCT_PREFIX/lib/libTKGeomBase.a" "$OCCT_PREFIX/lib/libTKBRep.a" \
  "$OCCT_PREFIX/lib/libTKGeomAlgo.a" "$OCCT_PREFIX/lib/libTKTopAlgo.a" \
  "$OCCT_PREFIX/lib/libTKShHealing.a" "$OCCT_PREFIX/lib/libTKPrim.a" \
  "$OCCT_PREFIX/lib/libTKBO.a" "$OCCT_PREFIX/lib/libTKBool.a" \
  "$OCCT_PREFIX/lib/libTKFillet.a" "$OCCT_PREFIX/lib/libTKOffset.a" \
  "$OCCT_PREFIX/lib/libTKMesh.a" \
  -Wl,--end-group -lpthread -ldl -lm -lrt -o lmcad
```

The OCCT recipe archive also carries the matching CMake configuration, C++
sources, embedded schema input, and full project/third-party notices. The
object archive is architecture-specific and reflects the schema and C++
source at its release. For other source changes, rebuild the application with
CMake using the modified `LMCAD_OCCT_PREFIX` and `LMCAD_FULL_STATIC=ON`. The
OCCT archives are grouped to resolve their static circular references. Keep the
replacement OCCT license and exception texts with any redistributed relinked
binary.
