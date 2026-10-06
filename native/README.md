# Native semantic core

This directory is the first executable mirror of the Idriç type sketch.

It is intentionally **not Android glue**.  The same C must run in a host test
and in an NDK shared library.  Android pointer IDs, lifecycle commands,
`ANativeWindow`, EGL/GLES/Sokol objects and signing/package details do not
belong here.

Current executable slice:

- normalized fixed rotation axis;
- signed, unwrapped physical angle;
- Spin(3) lift as a unit quaternion;
- projected SO(3) orientation matrix;
- distinct 0 / 2π / 4π pedagogical checkpoints;
- forward/reverse state transition.

This is deliberately smaller than `types/Spinor.idric`.  The ribbon
deformation field is the next semantic boundary; it should be imported from a
checked construction rather than improvised in the Android adapter.

Host check:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic \
  native/spinor_core.c native/test_spinor_core.c -lm \
  -o /tmp/spinor-core-test
/tmp/spinor-core-test
```

For Android, compile `spinor_core.c` into the application NativeActivity
library with the NDK.  Packaging should use the reusable
`isomorphisms/android-NDK` NativeActivity/APK route instead of copying that
machinery into this repository.
