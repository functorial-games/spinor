# Native semantic core

This directory is the executable mirror of the first part of the Idriç type
sketch.

It is intentionally **not Android glue**. The same C runs in host tests and in
an NDK shared library. Android pointer IDs, lifecycle commands,
`ANativeWindow`, EGL/GLES/Sokol objects and signing/package details do not
belong here.

## Current executable slices

### Spin state

`spinor_core.[ch]` owns:

- normalized fixed rotation axis;
- signed, unwrapped physical angle;
- Spin(3) lift as a unit quaternion;
- projected SO(3) orientation matrix;
- distinct 0 / 2π / 4π pedagogical checkpoints;
- forward/reverse state transition.

### Ambient deformation field

`spinor_field.[ch]` owns an analytic single-axis contraction field:

- the complete central body lies in one rigid core sphere;
- the exterior anchor sphere is fixed;
- the shell between them carries a continuous Spin(3) contraction;
- every point keeps its radius;
- every sphere receives one common rigid rotation;
- at 0 and 4π the **entire field** is identity;
- at 2π the body orientation has returned while the intermediate field remains
  nontrivial.

Because radius is preserved and one rotation is applied to every point on a
given sphere, the exact ambient map is injective. The ribbon geometry therefore
does not need a collision-generating cloth simulation merely to exhibit the
belt trick.

### Six-ribbon sampler

`spinor_ribbons.[ch]` generates six reference strips along ±x, ±y and ±z,
then deforms every vertex through the common ambient field.

The sampler uses caller-owned buffers. Allocation, GPU upload and rendering are
not semantic-core responsibilities.

The rigid field radius is required to contain the whole cube, including its
corners, so ribbon roots and the body cannot silently receive different
rotations.

## Host acceptance

Set `ICK` to the qualified native compiler described in
[`notes/division-migration.md`](../notes/division-migration.md). The maintained
test route requires it and uses the common native Makefile.

Run:

```sh
sh native/test-host.sh
```

The acceptance checks include:

- lift +1 at 0;
- lift −1 at 2π while SO(3) orientation has returned;
- lift +1 at 4π;
- fixed ribbon anchors;
- rigid ribbon roots;
- nontrivial interior ribbon geometry at 2π;
- all ribbon vertices returned at 4π;
- radius and same-sphere distance preservation;
- deterministic forward/reverse geometry;
- explicit capacity and invalid-field failures.

The host lane uses strict C11 warnings:

```
-std=c11 -Wall -Wextra -Werror -pedantic
```

## Android

For Android, compile these files into the application NativeActivity library.
Packaging should use the reusable `isomorphisms/android-NDK` NativeActivity/APK
route instead of copying that machinery into this repository.

The next Android file should consume these APIs; it should not reimplement the
Spin(3) contraction or ribbon sampler.
