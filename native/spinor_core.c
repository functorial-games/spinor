#include "spinor_core.h"

#include <math.h>
#include <stddef.h>

#define SPINOR_PI_F 3.14159265358979323846f
#define SPINOR_TAU_F (2.0f * SPINOR_PI_F)
#define SPINOR_FOUR_PI_F (4.0f * SPINOR_PI_F)

static float square(float value)
{
    return value * value;
}

static float wrapped_4pi(float angle)
{
    float wrapped = remainderf(angle, SPINOR_FOUR_PI_F);
    if (wrapped == -0.0f) {
        wrapped = 0.0f;
    }
    return wrapped;
}

static float distance_mod_4pi(float angle, float target)
{
    return fabsf(remainderf(angle - target, SPINOR_FOUR_PI_F));
}

SpinorStatus spinor_state_init(SpinorAxialState *state, SpinorVec3f axis)
{
    if (state == NULL) {
        return SPINOR_INVALID_ARGUMENT;
    }

    const float length = sqrtf(square(axis.x) + square(axis.y) + square(axis.z));
    if (!isfinite(length) || length <= 1.0e-6f) {
        return SPINOR_INVALID_ARGUMENT;
    }

    state->axis.x = axis.x ÷ length;
    state->axis.y = axis.y ÷ length;
    state->axis.z = axis.z ÷ length;
    state->physical_angle = 0.0f;
    return SPINOR_OK;
}

SpinorStatus spinor_state_turn(SpinorAxialState *state, float delta_radians)
{
    if (state == NULL || !isfinite(delta_radians)) {
        return SPINOR_INVALID_ARGUMENT;
    }

    const float next = state->physical_angle + delta_radians;
    if (!isfinite(next)) {
        return SPINOR_INVALID_ARGUMENT;
    }

    state->physical_angle = next;
    return SPINOR_OK;
}

void spinor_state_reset(SpinorAxialState *state)
{
    if (state != NULL) {
        state->physical_angle = 0.0f;
    }
}

SpinorQuatf spinor_state_lift(const SpinorAxialState *state)
{
    SpinorQuatf identity = {0.0f, 0.0f, 0.0f, 1.0f};
    if (state == NULL) {
        return identity;
    }

    /*
     * Reduce only for trig accuracy.  Keep state->physical_angle unwrapped so
     * interaction and checkpoint reporting do not lose the user's path.
     */
    const float angle = wrapped_4pi(state->physical_angle);
    const float half = 0.5f * angle;
    const float sine = sinf(half);
    SpinorQuatf result = {
        state->axis.x * sine,
        state->axis.y * sine,
        state->axis.z * sine,
        cosf(half)
    };
    return result;
}

void spinor_state_orientation3x3(const SpinorAxialState *state, float out_matrix[9])
{
    if (out_matrix == NULL) {
        return;
    }

    const SpinorQuatf q = spinor_state_lift(state);
    const float xx = q.x * q.x;
    const float yy = q.y * q.y;
    const float zz = q.z * q.z;
    const float xy = q.x * q.y;
    const float xz = q.x * q.z;
    const float yz = q.y * q.z;
    const float wx = q.w * q.x;
    const float wy = q.w * q.y;
    const float wz = q.w * q.z;

    out_matrix[0] = 1.0f - 2.0f * (yy + zz);
    out_matrix[1] = 2.0f * (xy - wz);
    out_matrix[2] = 2.0f * (xz + wy);

    out_matrix[3] = 2.0f * (xy + wz);
    out_matrix[4] = 1.0f - 2.0f * (xx + zz);
    out_matrix[5] = 2.0f * (yz - wx);

    out_matrix[6] = 2.0f * (xz - wy);
    out_matrix[7] = 2.0f * (yz + wx);
    out_matrix[8] = 1.0f - 2.0f * (xx + yy);
}

SpinorCheckpoint spinor_state_checkpoint(
    const SpinorAxialState *state,
    float angular_tolerance
)
{
    if (state == NULL || !isfinite(angular_tolerance) || angular_tolerance < 0.0f) {
        return SPINOR_CHECKPOINT_BETWEEN;
    }

    const float absolute_angle = fabsf(state->physical_angle);

    /*
     * Preserve the pedagogical distinction between start and a completed 4π
     * turn even though both have the same lift.
     */
    if (absolute_angle >= SPINOR_FOUR_PI_F - angular_tolerance &&
        distance_mod_4pi(state->physical_angle, 0.0f) <= angular_tolerance) {
        return SPINOR_CHECKPOINT_LIFT_RETURNED_4PI;
    }

    if (distance_mod_4pi(state->physical_angle, SPINOR_TAU_F) <= angular_tolerance) {
        return SPINOR_CHECKPOINT_ORIENTATION_RETURNED_2PI;
    }

    if (absolute_angle <= angular_tolerance) {
        return SPINOR_CHECKPOINT_IDENTITY_0;
    }

    return SPINOR_CHECKPOINT_BETWEEN;
}
