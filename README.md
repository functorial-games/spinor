# spinor

Interactive spinor and belt-trick playground.

The first concrete goal is direct manipulation: grab the central body or a ribbon control and move it, while the attached field/ribbons deform continuously enough to make the difference between a 2π turn and a 4π turn physically obvious.

The implementation starts from the semantic boundary rather than the renderer:

- [types/Spinor.idric](types/Spinor.idric) — Idriç type sketch for Spin(3), SO(3), lifted interaction state, ribbons, and later Seifert surfaces;
- [native/spinor_core.h](native/spinor_core.h) — first small C ABI mirrored from that sketch;
- [native/spinor_field.h](native/spinor_field.h) — analytic Spin(3) ambient contraction with rigid core and fixed exterior;
- [native/spinor_ribbons.h](native/spinor_ribbons.h) — six-ribbon geometry sampler over that field;
- [notes/idric-ndk-boundary.md](notes/idric-ndk-boundary.md) — why the Android/NDK boundary is shaped this way;
- [android/README.md](android/README.md) — NativeActivity lane and ownership boundary.

The reference notes separate three things that are easy to blur together:

- Jason Hise's original Maya/C++ animation pipeline and his later browser experiments;
- independent implementations that expose useful mathematics or rendering techniques;
- the interaction we actually want to build rather than merely replaying a canned belt-trick animation.

See:

- [notes/jason-hise.md](notes/jason-hise.md)
- [notes/related-implementations.md](notes/related-implementations.md)
- [notes/interaction.md](notes/interaction.md)

Repository home: `functorial-games/spinor`.

## Repeatable movie

The public 0 → 2π → 4π demo is generated from the same semantic Spin(3)
state, contraction field, and six-ribbon sampler used by the Android build.
The repository writes numbered PPM stills and an exact trajectory receipt;
Kitchen performs only the canonical stills → H.264/MP4 assembly.

See [movie/README.md](movie/README.md).
