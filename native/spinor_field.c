#include "spinor_field.h"

#include <math.h>
#include <stddef.h>

#define SPINOR_HALF_PI_F 1.57079632679489661923f

static float dot3(SpinorVec3f left, SpinorVec3f right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

static float norm3(SpinorVec3f value)
{
    return sqrtf(dot3(value, value));
}

static SpinorVec3f scale3(SpinorVec3f value, float scale)
{
    SpinorVec3f result = {
        value.x * scale,
        value.y * scale,
        value.z * scale
    };
    return result;
}

static SpinorVec3f subtract3(SpinorVec3f left, SpinorVec3f right)
{
    SpinorVec3f result = {
        left.x - right.x,
        left.y - right.y,
        left.z - right.z
    };
    return result;
}

static SpinorQuatf multiply_quaternions(SpinorQuatf left, SpinorQuatf right)
{
    SpinorQuatf result = {
        left.w * right.x + left.x * right.w + left.y * right.z - left.z * right.y,
        left.w * right.y - left.x * right.z + left.y * right.w + left.z * right.x,
        left.w * right.z + left.x * right.y - left.y * right.x + left.z * right.w,
        left.w * right.w - left.x * right.x - left.y * right.y - left.z * right.z
    };
    return result;
}

static SpinorQuatf normalize_quaternion(SpinorQuatf value)
{
    const float length = sqrtf(
        value.x * value.x +
        value.y * value.y +
        value.z * value.z +
        value.w * value.w
    );

    if (!isfinite(length) || length <= 1.0e-8f) {
        SpinorQuatf identity = {0.0f, 0.0f, 0.0f, 1.0f};
        return identity;
    }

    SpinorQuatf result = {
        value.x ÷ length,
        value.y ÷ length,
        value.z ÷ length,
        value.w ÷ length
    };
    return result;
}

SpinorStatus spinor_field_init(
    SpinorField *field,
    const SpinorAxialState *state,
    float core_radius,
    float anchor_radius,
    SpinorVec3f auxiliary_hint
)
{
    if (field == NULL || state == NULL ||
        !isfinite(core_radius) || !isfinite(anchor_radius) ||
        core_radius < 0.0f || anchor_radius <= core_radius) {
        return SPINOR_INVALID_ARGUMENT;
    }

    /*
     * Gram-Schmidt the supplied hint against the spin axis. Keeping the
     * auxiliary direction explicit makes the particular contraction visible
     * and selectable instead of hiding it inside the renderer.
     */
    const float projection = dot3(auxiliary_hint, state->axis);
    const SpinorVec3f perpendicular = subtract3(
        auxiliary_hint,
        scale3(state->axis, projection)
    );
    const float length = norm3(perpendicular);

    if (!isfinite(length) || length <= 1.0e-6f) {
        return SPINOR_INVALID_ARGUMENT;
    }

    field->core_radius = core_radius;
    field->anchor_radius = anchor_radius;
    field->auxiliary_axis = scale3(perpendicular, 1.0f ÷ length);
    return SPINOR_OK;
}

float spinor_field_core_weight(const SpinorField *field, float radius)
{
    if (field == NULL || !isfinite(radius) || radius < 0.0f) {
        return NAN;
    }

    if (radius <= field->core_radius) {
        return 1.0f;
    }
    if (radius >= field->anchor_radius) {
        return 0.0f;
    }

    /*
     * Smoothstep gives zero radial derivative at both rigid regions. The
     * topology does not depend on this profile; it only avoids a visible kink
     * where rigid core/fixed exterior meet the deforming shell.
     */
    const float t =
        (radius - field->core_radius) ÷
        (field->anchor_radius - field->core_radius);
    const float smooth = t * t * (3.0f - 2.0f * t);
    return 1.0f - smooth;
}

SpinorQuatf spinor_field_lift_at(
    const SpinorAxialState *state,
    const SpinorField *field,
    SpinorVec3f point
)
{
    SpinorQuatf identity = {0.0f, 0.0f, 0.0f, 1.0f};
    if (state == NULL || field == NULL) {
        return identity;
    }

    const float radius = norm3(point);
    const float core_weight = spinor_field_core_weight(field, radius);
    const SpinorQuatf core = spinor_state_lift(state);

    if (!isfinite(core_weight)) {
        return identity;
    }
    if (core_weight >= 1.0f) {
        return core;
    }
    if (core_weight <= 0.0f) {
        return identity;
    }

    /*
     * Explicit contraction in Spin(3) ≅ S^3.
     *
     * Let C be the core lift and m a unit pure quaternion perpendicular to the
     * spin axis. Put φ = π(1-h)/2, c = cos φ and d = sin φ. Then
     *
     *     A = c C + d m
     *     B = c - d m
     *     F = A B
     *
     * At the core h=1, F=C. At the anchor h=0, F=m(-m)=1.
     * When C=1 (0 or 4π physical rotation),
     * F=(c+dm)(c-dm)=1 at every radius.
     *
     * Thus 2π can carry a nontrivial field while the whole field returns
     * exactly to the identity after 4π.
     */
    const float phi = SPINOR_HALF_PI_F * (1.0f - core_weight);
    const float c = cosf(phi);
    const float d = sinf(phi);
    const SpinorVec3f m = field->auxiliary_axis;

    SpinorQuatf first = {
        c * core.x + d * m.x,
        c * core.y + d * m.y,
        c * core.z + d * m.z,
        c * core.w
    };
    SpinorQuatf second = {
        -d * m.x,
        -d * m.y,
        -d * m.z,
        c
    };

    return normalize_quaternion(multiply_quaternions(first, second));
}

SpinorVec3f spinor_field_deform_point(
    const SpinorAxialState *state,
    const SpinorField *field,
    SpinorVec3f point
)
{
    const SpinorQuatf q = spinor_field_lift_at(state, field, point);
    const SpinorQuatf vector = {point.x, point.y, point.z, 0.0f};
    const SpinorQuatf conjugate = {-q.x, -q.y, -q.z, q.w};
    const SpinorQuatf rotated = multiply_quaternions(
        multiply_quaternions(q, vector),
        conjugate
    );

    SpinorVec3f result = {rotated.x, rotated.y, rotated.z};
    return result;
}
