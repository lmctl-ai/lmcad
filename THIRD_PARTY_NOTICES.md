Third-party components in lmcad distribution
============================================

Open CASCADE Technology (OCCT) 8.0.1
------------------------------------
The Linux executable statically links OCCT toolkits TKernel, TKMath, TKG2d,
TKG3d, TKGeomBase, TKBRep, TKGeomAlgo, TKTopAlgo, TKShHealing, TKPrim,
TKBO, TKBool, TKFillet, TKOffset, and TKMesh. OCCT is copyright Open Cascade
SAS and distributed under GNU LGPL version 2.1 with the Open CASCADE exception.
This executable uses OCCT facilities. The corresponding source tag is V8_0_1:
https://github.com/Open-Cascade-SAS/OCCT/tree/V8_0_1
The license texts are LICENSE_LGPL_21.txt and OCCT_LGPL_EXCEPTION.txt in that
source distribution. CI release assets include both texts and an
`lmcad-occt-relinking-recipe.tar.gz` with the pinned build script and flags.
The source tag, toolchain details, and relinking instructions are described in
durable-memory/13-licensing.md and scripts/build-occt.sh.

GNU C Library and GNU compiler runtime
--------------------------------------
The fully static Linux binary includes the platform's glibc static objects
(including libc/libm/pthread/dl/rt where applicable), GNU libstdc++, and
libgcc. Their exact versions come from the GitHub-hosted Ubuntu runner image
and GCC shown in the CI run logs. glibc is LGPL-2.1-or-later; libstdc++ and
libgcc are GPLv3 with the GCC Runtime Library Exception. The runtime exception
permits use of these runtime libraries in application binaries; retain their
license and exception notices with any redistributed copies of the runtime
components. They are statically included in the executable, not shipped as
separate libraries.

Copied reference examples (source repository, not release binary)
-----------------------------------------------------------------
reference/python/b3d_*.py files are adapted/copied from build123d examples,
copyright their respective authors, Apache-2.0, with per-file source and
license headers. reference/python/pv_*.py files are copied/adapted from
PyVista examples, copyright their respective authors, MIT, with per-file
source and license headers. Those Python examples and their licenses are
source-tree materials; the binary release assets do not contain them.
Apache-2.0 requires retention of applicable copyright, patent, trademark,
and attribution notices and a copy of the license; MIT requires retention
of the copyright and permission notice. See each upstream project and the
license headers in the files for complete terms.

Vendored lmscript C++ runtime header
------------------------------------
cpp/dslvm.hpp is copied from lmctlhq/cpp-dsl-sdk at the commit recorded in
sdk.lock. This SDK repository currently provides no LICENSE file or explicit
redistribution grant. Its licensing status must be resolved by the SDK
maintainer before public redistribution. No license is asserted here.

The lmcad project license has not been selected. This notice inventories
third-party material only and does not grant rights in lmcad-authored code.
