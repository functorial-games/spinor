#ifndef SPINOR_RIBBONS_H
#define SPINOR_RIBBONS_H

#include "spinor_field.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPINOR_RIBBON_COUNT 6u
#define SPINOR_MAX_RIBBON_SEGMENTS 1024u

typedef struct {
    float body_half_extent;
    float width;
    uint32_t segments;
} SpinorRibbonSpec;

/*
 * Caller-owned output buffers keep allocation policy out of the semantic core.
 * uv_float_capacity counts floats, not vertex pairs.
 */
typedef struct {
    SpinorVec3f *positions;
    float *uv;
    uint32_t *indices;

    uint32_t vertex_capacity;
    uint32_t uv_float_capacity;
    uint32_t index_capacity;

    uint32_t vertex_count;
    uint32_t index_count;
} SpinorRibbonMesh;

uint32_t spinor_six_ribbon_vertex_count(uint32_t segments);
uint32_t spinor_six_ribbon_index_count(uint32_t segments);

SpinorStatus spinor_sample_six_ribbons(
    const SpinorAxialState *state,
    const SpinorField *field,
    const SpinorRibbonSpec *spec,
    SpinorRibbonMesh *mesh
);

#ifdef __cplusplus
}
#endif

#endif
