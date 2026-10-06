#include "spinor_ribbons.h"

#include <math.h>
#include <stddef.h>

static SpinorVec3f add3(SpinorVec3f left, SpinorVec3f right)
{
    SpinorVec3f result = {
        left.x + right.x,
        left.y + right.y,
        left.z + right.z
    };
    return result;
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

uint32_t spinor_six_ribbon_vertex_count(uint32_t segments)
{
    if (segments < 1u || segments > SPINOR_MAX_RIBBON_SEGMENTS) {
        return 0u;
    }
    return SPINOR_RIBBON_COUNT * (segments + 1u) * 2u;
}

uint32_t spinor_six_ribbon_index_count(uint32_t segments)
{
    if (segments < 1u || segments > SPINOR_MAX_RIBBON_SEGMENTS) {
        return 0u;
    }
    return SPINOR_RIBBON_COUNT * segments * 6u;
}

SpinorStatus spinor_sample_six_ribbons(
    const SpinorAxialState *state,
    const SpinorField *field,
    const SpinorRibbonSpec *spec,
    SpinorRibbonMesh *mesh
)
{
    if (state == NULL || field == NULL || spec == NULL || mesh == NULL ||
        mesh->positions == NULL || mesh->indices == NULL ||
        !isfinite(spec->body_half_extent) || !isfinite(spec->width) ||
        spec->body_half_extent <= 0.0f || spec->width <= 0.0f) {
        return SPINOR_INVALID_ARGUMENT;
    }

    const uint32_t vertex_count = spinor_six_ribbon_vertex_count(spec->segments);
    const uint32_t index_count = spinor_six_ribbon_index_count(spec->segments);

    if (vertex_count == 0u || index_count == 0u) {
        return SPINOR_LIMIT;
    }
    if (mesh->vertex_capacity < vertex_count ||
        mesh->index_capacity < index_count ||
        (mesh->uv != NULL && mesh->uv_float_capacity < 2u * vertex_count)) {
        return SPINOR_BUFFER_TOO_SMALL;
    }

    /*
     * The rigid core must contain the complete cube, not merely the six face
     * centers. This guarantees that all body geometry and ribbon roots receive
     * exactly the same SO(3) motion.
     */
    const float cube_corner_radius =
        sqrtf(3.0f) * spec->body_half_extent;
    const float ribbon_root_radius = sqrtf(
        spec->body_half_extent * spec->body_half_extent +
        0.25f * spec->width * spec->width
    );
    const float required_core_radius =
        fmaxf(cube_corner_radius, ribbon_root_radius);

    if (field->core_radius < required_core_radius ||
        field->anchor_radius <= spec->body_half_extent) {
        return SPINOR_INVALID_ARGUMENT;
    }

    /*
     * Six disjoint reference strips point through ±x, ±y and ±z. The same
     * ambient radial field deforms all six, rather than giving each ribbon a
     * private interpolation. Because every sphere is transformed by a single
     * rotation and radius is preserved, the exact field is injective.
     */
    const SpinorVec3f axis[SPINOR_RIBBON_COUNT] = {
        { 1.0f,  0.0f,  0.0f},
        {-1.0f,  0.0f,  0.0f},
        { 0.0f,  1.0f,  0.0f},
        { 0.0f, -1.0f,  0.0f},
        { 0.0f,  0.0f,  1.0f},
        { 0.0f,  0.0f, -1.0f}
    };
    const SpinorVec3f across[SPINOR_RIBBON_COUNT] = {
        { 0.0f,  1.0f,  0.0f},
        { 0.0f, -1.0f,  0.0f},
        { 0.0f,  0.0f,  1.0f},
        { 0.0f,  0.0f, -1.0f},
        { 1.0f,  0.0f,  0.0f},
        {-1.0f,  0.0f,  0.0f}
    };

    uint32_t vertex = 0u;
    uint32_t index = 0u;
    const float half_width = 0.5f * spec->width;

    for (uint32_t ribbon = 0u; ribbon < SPINOR_RIBBON_COUNT; ++ribbon) {
        const uint32_t base_vertex = vertex;

        for (uint32_t step = 0u; step <= spec->segments; ++step) {
            const float u = (float)step / (float)spec->segments;
            const float radius =
                spec->body_half_extent +
                u * (field->anchor_radius - spec->body_half_extent);
            const SpinorVec3f center = scale3(axis[ribbon], radius);

            for (uint32_t side = 0u; side < 2u; ++side) {
                const float lateral = side == 0u ? -half_width : half_width;
                const SpinorVec3f reference = add3(
                    center,
                    scale3(across[ribbon], lateral)
                );

                mesh->positions[vertex] =
                    spinor_field_deform_point(state, field, reference);

                if (mesh->uv != NULL) {
                    mesh->uv[2u * vertex] = u;
                    mesh->uv[2u * vertex + 1u] = (float)side;
                }
                ++vertex;
            }
        }

        for (uint32_t step = 0u; step < spec->segments; ++step) {
            const uint32_t a = base_vertex + 2u * step;
            const uint32_t b = a + 1u;
            const uint32_t c = a + 2u;
            const uint32_t d = a + 3u;

            mesh->indices[index++] = a;
            mesh->indices[index++] = c;
            mesh->indices[index++] = b;

            mesh->indices[index++] = b;
            mesh->indices[index++] = c;
            mesh->indices[index++] = d;
        }
    }

    mesh->vertex_count = vertex;
    mesh->index_count = index;
    return SPINOR_OK;
}
