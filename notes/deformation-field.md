# Analytic deformation field

The first executable ribbon field is intentionally simpler than a general
cloth solver or a full conformal-geometric-algebra motor interpolation.

It is an ambient deformation of 3-space built from a continuous Spin(3)
contraction.

## Boundary conditions

Let:

- (C(θ)) be the central body's Spin(3) lift;
- (h(r)=1) on the rigid core;
- (h(r)=0) on and outside the fixed anchor sphere;
- (m) be a unit pure quaternion perpendicular to the spin axis.

The implementation uses a smooth radial profile only to avoid a visible kink
at the two rigid regions. The topology does not depend on that particular
profile.

For a point at radius (r), put

[
φ = \frac{π}{2}(1-h(r)),\qquad
c=\cos φ,\qquad d=\sin φ,
]

and define

[
A = cC + dm,\qquad
B = c-dm,\qquad
F = AB.
]

`spinor_field_lift_at` normalizes the floating-point result.

## Required identities

At the rigid core, (h=1), hence (φ=0) and

[
F=C.
]

At the exterior anchor, (h=0), hence (φ=π/2) and

[
F=m(-m)=1.
]

At physical angle 0, (C=1), so for every radius

[
F=(c+dm)(c-dm)=1.
]

At physical angle 4π the Spin(3) lift is again (C=1), so **the whole ambient
field returns to identity**, not merely the central body.

At 2π, (C=-1). The central rigid SO(3) orientation is therefore the same as
at 0, while intermediate radii generally have (F\ne\pm1). That intermediate
field is the visible memory of the nontrivial turn.

## Why this is a useful first field

For fixed radius (r), every point on the sphere receives the same spatial
rotation induced by (F(r,θ)). Therefore:

- radius is preserved;
- distances between points on one sphere are preserved;
- a sphere maps bijectively to itself;
- different spheres cannot collide because their radii differ.

So the continuous ambient map is injective before tessellation. Six initially
disjoint strips remain disjoint under the exact deformation. Any visual
self-intersection introduced later is therefore a sampling/rendering defect,
not part of the mathematical field.

This property is valuable for the first phone interaction: we can rigorously
test the geometry without introducing a physics solver whose numerical
behavior obscures the topology.

## Relation to the references

This field is an application-owned analytic construction, not copied source
from the unlicensed `parallax1s/belt-trick` repository.

The separate MIT-licensed Amir Leidel `spinny` construction remains a useful
second implementation to compare later. It uses conformal geometric algebra
motor interpolation and can serve as an independent visual/mathematical
reference once the first interaction is working.

## Next boundary

The Android adapter should convert a horizontal body drag into a signed angle
increment and call:

```c
spinor_state_turn(...)
spinor_sample_six_ribbons(...)
```

It should then upload/draw the returned positions and indices.

No Android event handler should contain the contraction equations.
