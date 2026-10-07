# spinor

Interactive spinor and belt-trick playground.

## Credit and visual lineage

This project is directly and substantially inspired by **Jason Hise's belt-trick / spin-½ / antitwister visualizations**. The central pedagogical image here—a body turning through 2π and 4π while attached ribbons make the difference between the returned ordinary orientation and the returned lifted spinor state visible—belongs to the visual lineage Hise developed and popularized in his mathematical animations.

Hise's work is more than a generic reference to the Dirac belt trick. His animations developed the particular visual language of a central object coupled to multiple continuously deforming fibers/ribbons, and his later work extended the construction to many fibers so that a whole region of space could be seen twisting continuously without tangling. His 2016 Wikimedia essay also documents the production method behind those animations: procedural mathematical geometry implemented in a C++ Maya custom shape node, animated through its parameters, and rendered with an ordinary 3D pipeline. His current **Entropy Games** work continues that line with interactive browser visualizations, including antitwister and higher-dimensional geometry experiments.

Primary Hise references:

- [Jason Hise, “Why I create beautiful math GIFs” — Wikimedia Diff](https://diff.wikimedia.org/2016/09/22/math-gifs/)
- [JasonHise on Wikimedia Commons](https://commons.wikimedia.org/wiki/User:JasonHise)
- [Entropy Games](https://entropygames.net/)
- [Twist Gallery / antitwister work](https://entropygames.net/twist_gallery.html)
- [Detailed Hise research and provenance notes for this repository](notes/jason-hise.md)

The code in this repository is an independent implementation, not a copy of Hise's unreleased Maya/C++ source. Our current Spin(3) state model, analytic contraction field, six-ribbon sampler, Android renderer, and deterministic movie renderer are implemented here from mathematical descriptions and independently available references. That distinction matters for both provenance and credit: **the implementation is ours; the visual and pedagogical debt to Hise is explicit and substantial.**

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
