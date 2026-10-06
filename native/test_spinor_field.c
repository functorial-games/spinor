#include "spinor_ribbons.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI_F 3.14159265358979323846f
#define TAU_F (2.0f * PI_F)
#define FOUR_PI_F (4.0f * PI_F)
#define EPSILON_F 4.0e-5f

static int near(float left, float right)
{
    return fabsf(left - right) <= EPSILON_F;
}

static int same_point(SpinorVec3f left, SpinorVec3f right)
{
    return near(left.x, right.x) &&
           near(left.y, right.y) &&
           near(left.z, right.z);
}

static float norm3(SpinorVec3f value)
{
    return sqrtf(value.x * value.x + value.y * value.y + value.z * value.z);
}

static float distance3(SpinorVec3f left, SpinorVec3f right)
{
    SpinorVec3f difference = {
        left.x - right.x,
        left.y - right.y,
        left.z - right.z
    };
    return norm3(difference);
}

int main(void)
{
    SpinorAxialState state;
    assert(
        spinor_state_init(&state, (SpinorVec3f){0.0f, 0.0f, 1.0f}) ==
        SPINOR_OK
    );

    SpinorField field;
    assert(
        spinor_field_init(
            &field,
            &state,
            1.8f,
            5.0f,
            (SpinorVec3f){1.0f, 0.0f, 0.0f}
        ) == SPINOR_OK
    );

    assert(near(spinor_field_core_weight(&field, 1.0f), 1.0f));
    assert(near(spinor_field_core_weight(&field, 5.0f), 0.0f));
    assert(isnan(spinor_field_core_weight(&field, -1.0f)));

    const SpinorRibbonSpec spec = {
        .body_half_extent = 1.0f,
        .width = 0.24f,
        .segments = 32u
    };

    const uint32_t vertex_count =
        spinor_six_ribbon_vertex_count(spec.segments);
    const uint32_t index_count =
        spinor_six_ribbon_index_count(spec.segments);

    assert(vertex_count == 396u);
    assert(index_count == 1152u);

    SpinorVec3f *positions = calloc(vertex_count, sizeof(*positions));
    SpinorVec3f *baseline = calloc(vertex_count, sizeof(*baseline));
    float *uv = calloc((size_t)vertex_count * 2u, sizeof(*uv));
    uint32_t *indices = calloc(index_count, sizeof(*indices));

    assert(positions != NULL);
    assert(baseline != NULL);
    assert(uv != NULL);
    assert(indices != NULL);

    SpinorRibbonMesh mesh = {
        .positions = positions,
        .uv = uv,
        .indices = indices,
        .vertex_capacity = vertex_count,
        .uv_float_capacity = 2u * vertex_count,
        .index_capacity = index_count,
        .vertex_count = 0u,
        .index_count = 0u
    };

    /* At 0 the whole field is identity. */
    assert(
        spinor_sample_six_ribbons(&state, &field, &spec, &mesh) ==
        SPINOR_OK
    );
    memcpy(
        baseline,
        positions,
        (size_t)vertex_count * sizeof(*positions)
    );

    /* At 2π the rigid body orientation has returned but the field has not. */
    assert(spinor_state_turn(&state, TAU_F) == SPINOR_OK);

    const SpinorQuatf core = spinor_field_lift_at(
        &state,
        &field,
        (SpinorVec3f){0.0f, 0.0f, 1.0f}
    );
    assert(near(core.x, 0.0f));
    assert(near(core.y, 0.0f));
    assert(near(core.z, 0.0f));
    assert(near(core.w, -1.0f));

    const SpinorQuatf outer = spinor_field_lift_at(
        &state,
        &field,
        (SpinorVec3f){0.0f, 5.0f, 0.0f}
    );
    assert(near(outer.x, 0.0f));
    assert(near(outer.y, 0.0f));
    assert(near(outer.z, 0.0f));
    assert(near(outer.w, 1.0f));

    const SpinorVec3f middle = {0.0f, 3.0f, 0.0f};
    const SpinorVec3f moved =
        spinor_field_deform_point(&state, &field, middle);
    assert(!same_point(middle, moved));
    assert(near(norm3(middle), norm3(moved)));

    /*
     * Points on one radius sphere receive one common rotation, so distances
     * within that sphere are preserved as well as radius.
     */
    const SpinorVec3f sphere_a = {0.0f, 3.0f, 0.0f};
    const SpinorVec3f sphere_b = {0.0f, 0.0f, 3.0f};
    const SpinorVec3f deformed_a =
        spinor_field_deform_point(&state, &field, sphere_a);
    const SpinorVec3f deformed_b =
        spinor_field_deform_point(&state, &field, sphere_b);
    assert(near(distance3(sphere_a, sphere_b),
                distance3(deformed_a, deformed_b)));

    assert(
        spinor_sample_six_ribbons(&state, &field, &spec, &mesh) ==
        SPINOR_OK
    );

    int any_interior_vertex_moved = 0;
    const uint32_t vertices_per_ribbon = (spec.segments + 1u) * 2u;

    for (uint32_t ribbon = 0u; ribbon < SPINOR_RIBBON_COUNT; ++ribbon) {
        const uint32_t root = ribbon * vertices_per_ribbon;
        const uint32_t anchor = root + spec.segments * 2u;

        assert(same_point(positions[root], baseline[root]));
        assert(same_point(positions[root + 1u], baseline[root + 1u]));
        assert(same_point(positions[anchor], baseline[anchor]));
        assert(same_point(positions[anchor + 1u], baseline[anchor + 1u]));
    }

    for (uint32_t vertex = 0u; vertex < vertex_count; ++vertex) {
        if (!same_point(positions[vertex], baseline[vertex])) {
            any_interior_vertex_moved = 1;
            break;
        }
    }
    assert(any_interior_vertex_moved);

    /* At 4π every sampled ribbon vertex returns, not just the cube. */
    assert(spinor_state_turn(&state, TAU_F) == SPINOR_OK);
    assert(
        spinor_sample_six_ribbons(&state, &field, &spec, &mesh) ==
        SPINOR_OK
    );
    for (uint32_t vertex = 0u; vertex < vertex_count; ++vertex) {
        assert(same_point(positions[vertex], baseline[vertex]));
    }

    /* Forward then reverse must reproduce geometry deterministically. */
    assert(
        spinor_state_turn(&state, -FOUR_PI_F + 0.7f) ==
        SPINOR_OK
    );
    assert(
        spinor_sample_six_ribbons(&state, &field, &spec, &mesh) ==
        SPINOR_OK
    );

    SpinorVec3f *snapshot = malloc(
        (size_t)vertex_count * sizeof(*snapshot)
    );
    assert(snapshot != NULL);
    memcpy(
        snapshot,
        positions,
        (size_t)vertex_count * sizeof(*positions)
    );

    assert(spinor_state_turn(&state, 1.1f) == SPINOR_OK);
    assert(spinor_state_turn(&state, -1.1f) == SPINOR_OK);
    assert(
        spinor_sample_six_ribbons(&state, &field, &spec, &mesh) ==
        SPINOR_OK
    );

    for (uint32_t vertex = 0u; vertex < vertex_count; ++vertex) {
        assert(same_point(positions[vertex], snapshot[vertex]));
    }

    /* Capacity and construction failures remain explicit. */
    SpinorRibbonMesh short_mesh = mesh;
    short_mesh.index_capacity = index_count - 1u;
    assert(
        spinor_sample_six_ribbons(&state, &field, &spec, &short_mesh) ==
        SPINOR_BUFFER_TOO_SMALL
    );

    SpinorField invalid_field;
    assert(
        spinor_field_init(
            &invalid_field,
            &state,
            1.0f,
            2.0f,
            (SpinorVec3f){0.0f, 0.0f, 4.0f}
        ) == SPINOR_INVALID_ARGUMENT
    );

    free(snapshot);
    free(indices);
    free(uv);
    free(baseline);
    free(positions);

    puts("spinor_field: ok");
    return 0;
}
