# Idriç ↔ native/NDK boundary

## Decision

Use the Idriç file as the semantic/type sketch and keep the first executable
phone lane C-only:

```
types/Spinor.idric
        │ semantic contract
        ▼
native/spinor_core.[ch]
        │ small checked C ABI
        ▼
Android adapter (NativeActivity)
        │ touch/lifecycle/window only
        ▼
renderer
```

The initial MIRO A1 lane is `armeabi-v7a`, AArch32, 32-bit pointers, Android
softfp call ABI.  The core uses 32-bit floats intentionally; the double-cover
invariants need topology and continuity more than gratuitous per-value
precision.  Host tests use tolerances rather than false IEEE-754 equality
proofs.

## What prior projects taught us

### Young Tableaux

The type file successfully exposed the mathematical vocabulary and prevented
UI concepts from defining the mathematics.  The APK that actually worked was
still C-only NativeActivity code.  Its useful boundary was therefore **typed
design → small native domain structures → thin Android adapter**, not “Idriç
owns the whole APK”.

Spinor follows that result rather than the original ambition.

### Beauty

`FaceField.idric` explicitly says “No claim of compiled execution.”  That is a
good precedent.  A type sketch is useful even when current lowering cannot
execute every dependent/semantic idea.  We should not label design syntax as
runtime evidence.

### android-NDK

The reusable repository now states the correct ownership rule: applications
own mathematics, simulation, renderer and interaction semantics; android-NDK
owns reusable platform mechanics and APK routes.  Spinor should consume that
layer, not fork `android_native_app_glue`, signer logic, ABI knowledge or APK
assembly.

### DEX / earlier Idriç Android work

Direct Idriç→DEX proved a narrow checked language/backend path.  It did not
prove a general renderer, recursive mathematical model, array-rich C ABI or
Android UI lowering.  For this app, forcing the spinor mathematics through DEX
would add a boundary without helping the visual.

## Important correction specific to the belt trick

The central object's final SO(3) orientation is insufficient state.

At 2π:

- the ordinary rigid orientation has returned;
- the Spin(3) lift is −1;
- the field/ribbons must still carry the nontrivial state.

Therefore the native core stores an **unwrapped physical angle** for the first
single-axis slice and derives both:

- `Spin3 lift`;
- projected `SO3 orientation`.

Later free 3D grabbing should store a continuous lifted path/continuation, not
canonicalize each frame back to a bare SO(3) quaternion and thereby lose the
path.

## NDK/platform boundary

The future NativeActivity file should do only this:

1. receive `AInputEvent` / lifecycle events;
2. own the active Android pointer ID and pixel coordinates;
3. turn drag motion into a semantic signed angle increment;
4. call `spinor_state_turn`;
5. ask the deformation sampler/renderer to redraw;
6. keep camera gestures separate from object-spin gestures.

The Android adapter must **not**:

- invent the 2π/4π semantics;
- reconstruct the lift from the final SO(3) orientation;
- own ribbon topology;
- contain Hise/Leidel deformation math;
- persist pointers/GL objects as saved state.

## Ribbon field next

The next semantic addition should be a deformation-field implementation with
host-visible fixtures.  The type sketch already reserves `DeformationField`,
`BeltSystem` and `RibbonMesh`.

Do not use a cloth solver for the first slice.  Compare a licensed
Hise-inspired/Leidel construction against an independently checked field, then
put only the chosen mathematical sampler behind the C boundary.

## Seifert surfaces later

Seifert surfaces belong above the renderer for the same reason.  Their
orientation and boundary-link relation are semantic; tessellation, shading and
GPU upload are implementation.  The sketch records the type now but the first
six-belt ABI does not pretend to implement it.
