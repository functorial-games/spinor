# Division compiler routes

The 26 ordinary binary divisions in maintained native C use literal `÷`.
ICK c61e448251744a2f40ad743ebef1a027bdcd2f9d keeps their integer/floating
semantics. The embedded GLES shader, strings and comments retain their bytes.

Host semantic tests and movie stills require explicit `ICK`; the shared
native Makefile retains the warnings and optimization settings and adds the
qualified `-fno-link-libatomic` driver option. Kitchen still owns only the
stills-to-MP4 assembly. The PR workflow builds that same movie without
uploading assets to the existing release.

The Android script requires `ICK_CC` or `ICK_ROOT` for its selected ABI.
All five owned C translation units pass through ICK; NDK r27c assembles,
compiles unchanged upstream native_app_glue, and links the original API21
library. The maintained ARMv7/AArch64 matrix, strict warnings, ELF page
alignment and NativeActivity entry points remain. Compiler builtin headers
precede Bionic headers and both Android API macros agree.

All shared compiler actions are pinned to ai-ci
903b2cb27ea572c9c6cb2ffa9f39e0fbf06ec9f8; the C-stage contract is required.
Local semantic tests pass. Sixteen deterministic 64×64 stills, their checksums
and the trajectory receipt are byte-identical to the original ASCII source.
The complete AArch64/API21 library compiles with warnings as errors, assembles
and links against actual NDK 27.2.12479018, with both required entry points.
Hosted workflows additionally build the maintained signed APKs and full movie.
These checks do not establish installation, touch/lifecycle behavior or
physical device acceptance.
