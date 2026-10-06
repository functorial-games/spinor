# Android lane

Spinor should use the existing reusable NativeActivity/APK route in
`isomorphisms/android-NDK`.

First target:

- MIRO A1
- `armeabi-v7a`
- NativeActivity
- application `classes.dex`: absent
- app/domain code: C
- no Java/Kotlin/Gradle requirement
- Android adapter owns lifecycle, touch pointer IDs and the native window
- `native/spinor_core.[ch]` owns the spin/lift semantics
- renderer remains a separate application-owned layer

No local copy of `android_native_app_glue` or generic APK/signing scripts
should be added here.  Consume the canonical android-NDK source/build route.

The first Android implementation should not be written until the deformation
field has a host-tested C boundary: otherwise the platform file becomes the
place where mathematical guesses accumulate.
