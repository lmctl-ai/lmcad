Third-party components in lmcad distribution
============================================

Project license
---------------
The lmcad-authored source is licensed under the MIT License; see `LICENSE`.
This includes the vendored `cpp/dslvm.hpp`, which originates from the
MIT-licensed cpp-dsl-sdk, and lmcad's own C++ code and tools.

Open CASCADE Technology (OCCT) 8.0.1
------------------------------------
The Linux executable statically links OCCT toolkits TKernel, TKMath, TKG2d,
TKG3d, TKGeomBase, TKBRep, TKGeomAlgo, TKTopAlgo, TKShHealing, TKPrim,
TKBO, TKBool, TKFillet, TKOffset, and TKMesh. OCCT is copyright Open Cascade
SAS and distributed under GNU LGPL version 2.1 with the Open CASCADE exception.
The source tag is V8_0_1:
https://github.com/Open-Cascade-SAS/OCCT/tree/V8_0_1
The release includes OCCT's exact LGPL and exception texts, plus
`OCCT-RELINKING.md`, an architecture-specific archive of lmcad link objects,
and `lmcad-occt-relinking-recipe.tar.gz` with `scripts/build-occt.sh`, the pinned
configure flags, and both architecture-specific object archives. The object
archives and link recipe allow rebuilding
modified OCCT archives and relinking the static application; see the included
instructions.

GNU C Library and GNU compiler runtime
--------------------------------------
The fully static Linux binary includes the runner's glibc static objects
(including libc/libm/pthread/dl/rt where applicable), GNU libstdc++, and
libgcc. glibc is LGPL-2.1-or-later. libstdc++ and libgcc are GPLv3-or-later
with GCC Runtime Library Exception 3.1; that exception permits eligible
compiled application code to be conveyed under its own terms. The release
includes LGPL-2.1, GPL-3, the GCC runtime exception text, and the copyright
metadata for the runner's GCC and glibc packages. The static link objects and
instructions are supplied for relinking with modified libraries.

Copied reference examples (source repository)
---------------------------------------------
`reference/python/b3d_*.py` files are adapted/copied from build123d examples,
Apache-2.0, with per-file source and license headers. The full Apache-2.0
license and build123d's upstream NOTICE are included in release materials.
The NOTICE attributes build123d to its contributors, notes its historical
CadQuery derivation, and states its Apache-2.0 licensing.

`reference/python/pv_*.py` files are copied/adapted from PyVista examples,
MIT, with per-file source and license headers. PyVista's full MIT license is
included in release materials. These Python reference scripts are source-tree
materials and are not linked into the native executable.

The former `b3d_04_logo.py` reference and its generated PNG/STL/SVG outputs
were removed. The script requested Arial, while its generated outlines did
not record which system font was resolved; the output font license could not
be verified reproducibly.
