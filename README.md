# spinor

Interactive spinor and belt-trick playground.

The first concrete goal is direct manipulation: grab the central body or a ribbon control and move it, while the attached field/ribbons deform continuously enough to make the difference between a 2π turn and a 4π turn physically obvious.

This repository begins with research rather than a renderer. The reference notes separate three things that are easy to blur together:

- Jason Hise's original Maya/C++ animation pipeline and his later browser experiments;
- independent implementations that expose useful mathematics or rendering techniques;
- the interaction we actually want to build rather than merely replaying a canned belt-trick animation.

See:

- [notes/jason-hise.md](notes/jason-hise.md)
- [notes/related-implementations.md](notes/related-implementations.md)
- [notes/interaction.md](notes/interaction.md)

The intended home of this repository is the `functorial-games` organization.
