#ifndef SPINOR_FIELD_H
#define SPINOR_FIELD_H

#include "spinor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Analytic contraction field for the first single-axis belt-trick slice.
 *
 * All points with radius <= core_radius receive exactly the body's Spin(3)
 * lift. All points with radius >= anchor_radius are fixed. Between those
 * spheres, a continuous Spin(3) contraction connects the two.
 */
typedef struct {
    float core_radius;
    float anchor_radius;
    SpinorVec3f auxiliary_axis; /* unit and perpendicular to the spin axis */
} SpinorField;

SpinorStatus spinor_field_init(
    SpinorField *field,
    const SpinorAxialState *state,
    float core_radius,
    float anchor_radius,
    SpinorVec3f auxiliary_hint
);

/* Diagnostic/profile function. Radius must be finite and nonnegative. */
float spinor_field_core_weight(const SpinorField *field, float radius);

SpinorQuatf spinor_field_lift_at(
    const SpinorAxialState *state,
    const SpinorField *field,
    SpinorVec3f point
);

SpinorVec3f spinor_field_deform_point(
    const SpinorAxialState *state,
    const SpinorField *field,
    SpinorVec3f point
);

#ifdef __cplusplus
}
#endif

#endif
