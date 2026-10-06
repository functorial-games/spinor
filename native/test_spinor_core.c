#include "spinor_core.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#define PI_F 3.14159265358979323846f
#define TAU_F (2.0f * PI_F)
#define FOUR_PI_F (4.0f * PI_F)
#define EPS 1.0e-5f

static int near(float left, float right)
{
    return fabsf(left - right) <= EPS;
}

static void expect_same_matrix(const float left[9], const float right[9])
{
    for (int index = 0; index < 9; ++index) {
        assert(near(left[index], right[index]));
    }
}

int main(void)
{
    SpinorAxialState state;
    assert(spinor_state_init(&state, (SpinorVec3f){0.0f, 0.0f, 3.0f}) == SPINOR_OK);

    SpinorQuatf q0 = spinor_state_lift(&state);
    assert(near(q0.x, 0.0f));
    assert(near(q0.y, 0.0f));
    assert(near(q0.z, 0.0f));
    assert(near(q0.w, 1.0f));
    assert(spinor_state_checkpoint(&state, EPS) == SPINOR_CHECKPOINT_IDENTITY_0);

    float orientation0[9];
    spinor_state_orientation3x3(&state, orientation0);

    assert(spinor_state_turn(&state, TAU_F) == SPINOR_OK);
    SpinorQuatf q2pi = spinor_state_lift(&state);
    assert(near(q2pi.x, 0.0f));
    assert(near(q2pi.y, 0.0f));
    assert(near(q2pi.z, 0.0f));
    assert(near(q2pi.w, -1.0f));
    assert(spinor_state_checkpoint(&state, EPS) ==
           SPINOR_CHECKPOINT_ORIENTATION_RETURNED_2PI);

    float orientation2pi[9];
    spinor_state_orientation3x3(&state, orientation2pi);
    expect_same_matrix(orientation0, orientation2pi);

    assert(spinor_state_turn(&state, TAU_F) == SPINOR_OK);
    SpinorQuatf q4pi = spinor_state_lift(&state);
    assert(near(q4pi.x, 0.0f));
    assert(near(q4pi.y, 0.0f));
    assert(near(q4pi.z, 0.0f));
    assert(near(q4pi.w, 1.0f));
    assert(spinor_state_checkpoint(&state, EPS) ==
           SPINOR_CHECKPOINT_LIFT_RETURNED_4PI);

    assert(spinor_state_turn(&state, -FOUR_PI_F) == SPINOR_OK);
    SpinorQuatf reversed = spinor_state_lift(&state);
    assert(near(reversed.w, 1.0f));
    assert(spinor_state_checkpoint(&state, EPS) == SPINOR_CHECKPOINT_IDENTITY_0);

    SpinorAxialState invalid;
    assert(spinor_state_init(&invalid, (SpinorVec3f){0.0f, 0.0f, 0.0f}) ==
           SPINOR_INVALID_ARGUMENT);

    puts("spinor_core: ok");
    return 0;
}
