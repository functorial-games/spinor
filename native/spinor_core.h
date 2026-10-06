#ifndef SPINOR_CORE_H
#define SPINOR_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * First executable slice of the Idriç type sketch.
 *
 * This is application/domain C, not Android glue. It is deliberately safe to
 * host-test. The Android NativeActivity adapter should translate touch into
 * signed turn increments and call this API.
 */

typedef struct {
    float x;
    float y;
    float z;
} SpinorVec3f;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} SpinorQuatf;

typedef struct {
    SpinorVec3f axis;       /* normalized by spinor_state_init */
    float physical_angle;   /* signed, unwrapped radians */
} SpinorAxialState;

typedef enum {
    SPINOR_OK = 0,
    SPINOR_INVALID_ARGUMENT = 1,
    SPINOR_BUFFER_TOO_SMALL = 2,
    SPINOR_LIMIT = 3
} SpinorStatus;

typedef enum {
    SPINOR_CHECKPOINT_BETWEEN = 0,
    SPINOR_CHECKPOINT_IDENTITY_0 = 1,
    SPINOR_CHECKPOINT_ORIENTATION_RETURNED_2PI = 2,
    SPINOR_CHECKPOINT_LIFT_RETURNED_4PI = 3
} SpinorCheckpoint;

SpinorStatus spinor_state_init(SpinorAxialState *state, SpinorVec3f axis);
SpinorStatus spinor_state_turn(SpinorAxialState *state, float delta_radians);
void spinor_state_reset(SpinorAxialState *state);

SpinorQuatf spinor_state_lift(const SpinorAxialState *state);
void spinor_state_orientation3x3(const SpinorAxialState *state, float out_matrix[9]);

/*
 * Classify pedagogical milestones modulo 4π with caller-selected angular
 * tolerance. 4π is checked before 0 because physical_angle is unwrapped.
 */
SpinorCheckpoint spinor_state_checkpoint(
    const SpinorAxialState *state,
    float angular_tolerance
);

#ifdef __cplusplus
}
#endif

#endif
