# Third-party licensing and redistribution

This note inventories third-party material in the lmcad source and Linux
release executable. It is an engineering inventory, not a legal conclusion.
The operator has not selected lmcad's own license; do not add a repository
`LICENSE` until that decision is made.

## Statically linked Linux components

- **Open CASCADE Technology 8.0.1:** the Linux executable statically links
  `TKernel`, `TKMath`, `TKG2d`, `TKG3d`, `TKGeomBase`, `TKBRep`, `TKGeomAlgo`,
  `TKTopAlgo`, `TKShHealing`, `TKPrim`, `TKBO`, `TKBool`, `TKFillet`,
  `TKOffset`, and `TKMesh`. OCCT is LGPL-2.1 with the Open CASCADE exception;
  source is pinned to `V8_0_1`. Include both upstream license files and
  prominent attribution that the binary uses OCCT. Because the build links
  OCCT statically, the distribution must provide an effective way to relink
  the work with a modified OCCT. This project uses the recipe route: this
  repository contains [`scripts/build-occt.sh`](../scripts/build-occt.sh),
  [`.github/occt-cmake-flags.txt`](../.github/occt-cmake-flags.txt), and this
  pinned source/build description. A release's source and recipe must remain
  available alongside its binary for the applicable license period; review
  the exact obligations against the LGPL-2.1 text and OCCT exception before
  release. OCCT's upstream CMake install places `LICENSE_LGPL_21.txt` and
  `OCCT_LGPL_EXCEPTION.txt` in its documentation directory.
- **glibc system archives:** `-static` includes the Ubuntu runner's static
  libc and related `libm`, pthread, dl, and rt objects as used by the link.
  Track the Ubuntu image/toolchain version in CI logs and preserve its
  corresponding notices/source package references when distributing the
  executable.
- **GNU libstdc++ and libgcc:** statically linked from the runner GCC toolchain;
  GNU GPLv3 plus the GCC Runtime Library Exception. The runtime exception
  permits these runtimes to be used in application binaries. Keep the
  applicable license and runtime-exception notices with release materials.
- **No other external libraries are configured into this Linux link closure.**
  The project uses the system C++ standard library, OCCT, and OS interfaces;
  PNG/DEFLATE, STL/OBJ, and APNG utilities are in-tree implementations.

## Copied reference source

- `reference/python/b3d_01_holes.py`, `b3d_02_lego.py`, `b3d_03_vase.py`,
  `b3d_04_logo.py`, `b3d_05_tea_cup.py`, and `b3d_06_joints.py`: build123d
  examples, Apache-2.0, marked with source URL, pinned upstream checkout, and
  license header. Preserve those
  notices; Apache-2.0 redistribution requires the license and applicable
  attribution/notices, and modified files must be marked if modified.
- `reference/python/pv_01_geometric_objects.py`, `pv_02_boolean_operations.py`,
  `pv_03_clip_with_plane_box.py`, `pv_04_light_types.py`, `pv_05_glyph.py`,
  and `pv_06_marching_cubes.py`: PyVista examples, MIT, marked with source URL,
  upstream revision and license header. Preserve the copyright and permission
  notice. These Python reference files are not part of the native release
  assets, but remain in the source repository.
- `cpp/dslvm.hpp`: copied from the pinned `cpp-dsl-sdk` commit in `sdk.lock`.
  At this revision the SDK has no `LICENSE` file or explicit license grant.
  This is an unresolved redistribution blocker for public binaries until the
  SDK maintainer clarifies licensing. `THIRD_PARTY_NOTICES` records that
  status without inventing a license.

## Release notice and relinking recipe

The tag release job copies [`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md)
to `dist/THIRD_PARTY_NOTICES` and attaches it beside both Linux binaries. The
release assets also include the exact OCCT LGPL-2.1 and exception files from
the cached install prefix, the runner's LGPL/GPL and copyright notices, and an
`lmcad-occt-relinking-recipe.tar.gz`. The private, unpublished npm layout also
includes these notices in its package folders. Do not publish the npm layout
while `license` remains `UNLICENSED` and `private: true`.

The OCCT configure flags are stored in `.github/occt-cmake-flags.txt`; CI hashes
that file in its cache key. The cold build script checks out tag `V8_0_1`,
builds the static libraries with `nice -n 10` and three parallel jobs, and
installs into a cacheable prefix. That script and its pinned tag/configuration
are the relinking recipe for the OCCT archive set. Retain the matching source
availability, compiler/toolchain identification, CMake options, and release
binary together. The release recipe archive contains `scripts/build-occt.sh`,
the hashed `.github/occt-cmake-flags.txt`, and this licensing note. See the
primary upstream license files:

- [OCCT README/license statement](https://github.com/Open-Cascade-SAS/OCCT#license)
- [OCCT LGPL-2.1 and exception](https://github.com/Open-Cascade-SAS/OCCT/blob/V8_0_1/OCCT_LGPL_EXCEPTION.txt)
- [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0.html)
- [MIT License](https://opensource.org/license/mit)
- [GCC libstdc++ runtime license and exception](https://gcc.gnu.org/onlinedocs/gcc-13.3.0/libstdc++/manual/manual/license.html)
