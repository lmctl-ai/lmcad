# 13 — Licensing and redistribution

The operator selected MIT for lmcad-authored code. The exact `LICENSE` text is
copied from `lmctl-ai/lmcpp` and states `Copyright (c) 2026 Lattice Systems
LLC`. No email address is included in the project license, README or npm
metadata. The MIT license covers lmcad's own code and its vendored
`cpp/dslvm.hpp`, now from the MIT-licensed cpp-dsl-sdk. No separate notice is
needed for the SDK header or the SDK's `stdlib/` catalog.

## Third-party components

- **OCCT 8.0.1:** Linux executables statically link TKernel, TKMath, TKG2d,
  TKG3d, TKGeomBase, TKBRep, TKGeomAlgo, TKTopAlgo, TKShHealing, TKPrim,
  TKBO, TKBool, TKFillet, TKOffset, and TKMesh. OCCT is LGPL-2.1 with the
  Open CASCADE exception. The exact upstream files are staged as
  `OCCT-LICENSE_LGPL_21.txt` and `OCCT-LGPL-EXCEPTION.txt`. The binary
  artifact also contains six lmcad application object files in
  `lmcad-linux-<arch>-relink-objects.tar.gz`; `OCCT-RELINKING.md` gives the
  matching static link command. `lmcad-occt-relinking-recipe.tar.gz` includes
  `scripts/build-occt.sh`, `.github/occt-cmake-flags.txt`, the link guide,
  both architecture-specific object archives, CMake configuration, and the
  source/notice material. This supports replacing
  modified OCCT archives and relinking the application. Preserve the upstream
  license and exception alongside any redistributed version.
- **glibc:** fully static Linux binaries include glibc static objects. glibc is
  LGPL-2.1-or-later. Artifacts include the LGPL-2.1 text and the runner's
  glibc package copyright metadata; application object files and link
  instructions are included for relinking against modified libraries.
- **libstdc++ and libgcc:** these static GCC runtime libraries are GPLv3-or-later
  with GCC Runtime Library Exception 3.1. The artifact includes GPL-3, the
  full GCC runtime exception text, and GCC package copyright metadata.
- **build123d references:** the remaining `reference/python/b3d_*.py` scripts
  are copied/adapted from the pinned `gumyr/build123d` examples at commit
  `092c40e3`, Apache-2.0. `licenses/Apache-2.0.txt` is the full upstream
  license and `licenses/build123d-NOTICE.txt` is the NOTICE from that exact
  upstream checkout. Preserve their source headers and the NOTICE statement.
- **PyVista references:** `reference/python/pv_*.py` scripts are copied/adapted
  from `pyvista/pyvista` commit `e9ae86b16`, MIT. The full license from that
  checkout is `licenses/PyVista-MIT.txt`.
- **Font-derived logo reference:** `b3d_04_logo.py` called `Text()` with
  `font="Arial"`; this system's fontconfig resolves that alias to DejaVu Sans,
  but the committed outlines do not record their resolved font. To avoid
  redistributing unverified font-derived outlines, the script and all three
  committed outputs (PNG, STL, SVG) were removed, along with the ranking entry.

`THIRD_PARTY_NOTICES.md` summarizes the components. The Linux stage script
puts the project `LICENSE`, `THIRD_PARTY_NOTICES`, and full upstream/system
license texts beside each binary. Each binary artifact also includes its
matching relink-object archive and `OCCT-RELINKING.md`. The release job's
OCCT recipe archive carries `LICENSE` and `THIRD_PARTY_NOTICES` too. The
private, unpublished npm packages declare `license: MIT` and each package
contains `LICENSE`, `THIRD_PARTY_NOTICES`, and all applicable notices. They
remain `private: true` and must not be published yet.

## Relinking recipe

The application objects are produced by CMake under
`build/CMakeFiles/lmcad.dir/cpp/`. `scripts/stage-release-notices.sh` archives
those six objects with the project license, notices and link guide. For an
OCCT modification, run `scripts/build-occt.sh` with `OCCT_SRC`, `OCCT_BUILD`
and `OCCT_PREFIX` set, using `.github/occt-cmake-flags.txt`; then follow
`licenses/LINUX-RELINKING.md` to link the supplied objects against the modified
15-archive OCCT closure with GCC's static C++ runtime and Linux system
libraries. For changed lmcad source, rebuild through CMake with
`-DLMCAD_OCCT_PREFIX=<modified-prefix> -DLMCAD_FULL_STATIC=ON`.

The OCCT configure flags are part of the CI cache key. CI caches the installed
prefix by OCCT version, flags hash, architecture and exact GCC version. The
measured cold build in run 39264695761 was 34m24s on x64 and 20m58s on arm64;
warm run 39270104147 restored both prefixes and completed the full jobs in 76s
and 74s. The tag release job was inspected but has not yet been exercised by a
real tag. macOS and Windows remain gated placeholders pending platform ports.
