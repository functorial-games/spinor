#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../native/spinor_core.h"
#include "../native/spinor_field.h"
#include "../native/spinor_ribbons.h"

#define PI_F 3.14159265358979323846f
#define TAU_F (2.0f * PI_F)
#define SEGMENTS 48u
#define VERTEX_COUNT (SPINOR_RIBBON_COUNT * (SEGMENTS + 1u) * 2u)
#define INDEX_COUNT (SPINOR_RIBBON_COUNT * SEGMENTS * 6u)

typedef struct {
    float x;
    float y;
    float depth;
} ScreenPoint;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

typedef struct {
    ScreenPoint a;
    ScreenPoint b;
    ScreenPoint c;
    Color color;
    float depth;
} Triangle;

static const Color ribbon_colors[SPINOR_RIBBON_COUNT] = {
    {235, 66, 56},
    {168, 31, 31},
    {77, 219, 97},
    {26, 148, 46},
    {66, 122, 250},
    {31, 61, 184}
};

static const Color cube_colors[6] = {
    {245, 222, 140},
    {209, 179, 92},
    {235, 207, 122},
    {184, 156, 82},
    {224, 194, 107},
    {199, 168, 87}
};

static float edge(ScreenPoint a, ScreenPoint b, float x, float y)
{
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

static ScreenPoint project(SpinorVec3f p, int width, int height)
{
    const float cy = 0.8525245221f;
    const float sy = 0.5226872289f;
    const float cp = 0.9004471024f;
    const float sp = -0.4349655341f;
    const float qx = cy * p.x + sy * p.z;
    const float qy = p.y;
    const float qz = -sy * p.x + cy * p.z;
    const float vx = qx;
    const float vy = cp * qy - sp * qz;
    const float vz = sp * qy + cp * qz - 18.0f;
    const float focal = 1.15f;
    const float aspect = (float)width ÷ (float)height;
    const float inverse_depth = 1.0f ÷ fmaxf(-vz, 0.001f);
    const float nx =
        focal * vx * inverse_depth ÷ fmaxf(aspect, 0.01f);
    const float ny = focal * vy * inverse_depth;

    ScreenPoint out = {
        (0.5f + 0.5f * nx) * (float)(width - 1),
        (0.5f - 0.5f * ny) * (float)(height - 1),
        -vz
    };
    return out;
}

static int compare_triangle_depth(const void *left, const void *right)
{
    const Triangle *a = left;
    const Triangle *b = right;

    if (a->depth < b->depth) {
        return 1;
    }
    if (a->depth > b->depth) {
        return -1;
    }
    return 0;
}

static void draw_triangle(
    uint8_t *pixels,
    int width,
    int height,
    const Triangle *triangle
)
{
    const float minxf = fminf(
        triangle->a.x,
        fminf(triangle->b.x, triangle->c.x)
    );
    const float maxxf = fmaxf(
        triangle->a.x,
        fmaxf(triangle->b.x, triangle->c.x)
    );
    const float minyf = fminf(
        triangle->a.y,
        fminf(triangle->b.y, triangle->c.y)
    );
    const float maxyf = fmaxf(
        triangle->a.y,
        fmaxf(triangle->b.y, triangle->c.y)
    );

    int minx = (int)floorf(minxf);
    int maxx = (int)ceilf(maxxf);
    int miny = (int)floorf(minyf);
    int maxy = (int)ceilf(maxyf);

    if (minx < 0) {
        minx = 0;
    }
    if (miny < 0) {
        miny = 0;
    }
    if (maxx >= width) {
        maxx = width - 1;
    }
    if (maxy >= height) {
        maxy = height - 1;
    }

    const float area = edge(
        triangle->a,
        triangle->b,
        triangle->c.x,
        triangle->c.y
    );
    if (fabsf(area) < 1.0e-5f) {
        return;
    }

    for (int y = miny; y <= maxy; ++y) {
        for (int x = minx; x <= maxx; ++x) {
            const float px = (float)x + 0.5f;
            const float py = (float)y + 0.5f;
            const float w0 = edge(
                triangle->b,
                triangle->c,
                px,
                py
            );
            const float w1 = edge(
                triangle->c,
                triangle->a,
                px,
                py
            );
            const float w2 = edge(
                triangle->a,
                triangle->b,
                px,
                py
            );

            const int inside =
                (area > 0.0f &&
                 w0 >= 0.0f &&
                 w1 >= 0.0f &&
                 w2 >= 0.0f) ||
                (area < 0.0f &&
                 w0 <= 0.0f &&
                 w1 <= 0.0f &&
                 w2 <= 0.0f);

            if (inside) {
                uint8_t *pixel =
                    pixels +
                    3u * (
                        (size_t)y * (size_t)width +
                        (size_t)x
                    );
                pixel[0] = triangle->color.r;
                pixel[1] = triangle->color.g;
                pixel[2] = triangle->color.b;
            }
        }
    }
}

static float trajectory_angle(uint32_t frame, uint32_t frame_count)
{
    if (frame_count <= 1u) {
        return 0.0f;
    }

    const float time =
        (float)frame ÷ (float)(frame_count - 1u);

    /*
     * Eight-second default movie:
     *
     *   0 -> 2pi    3 seconds
     *   hold 2pi    1 second
     *   2pi -> 4pi  3 seconds
     *   hold 4pi    1 second
     *
     * The hold at 2pi makes the orientation-returned/lift-not-returned
     * distinction visible without introducing another state machine.
     */
    if (time < 0.375f) {
        return TAU_F * (time ÷ 0.375f);
    }
    if (time < 0.500f) {
        return TAU_F;
    }
    if (time < 0.875f) {
        return
            TAU_F +
            TAU_F * ((time - 0.500f) ÷ 0.375f);
    }
    return 2.0f * TAU_F;
}

static const char *checkpoint_name(SpinorCheckpoint checkpoint)
{
    switch (checkpoint) {
        case SPINOR_CHECKPOINT_IDENTITY_0:
            return "0";
        case SPINOR_CHECKPOINT_ORIENTATION_RETURNED_2PI:
            return "2pi";
        case SPINOR_CHECKPOINT_LIFT_RETURNED_4PI:
            return "4pi";
        default:
            return "between";
    }
}

static int render_frame(
    const char *path,
    float angle,
    int width,
    int height,
    SpinorAxialState *state,
    SpinorField *field
)
{
    SpinorVec3f positions[VERTEX_COUNT];
    uint32_t indices[INDEX_COUNT];
    SpinorRibbonSpec ribbon_spec = {
        1.0f,
        0.28f,
        SEGMENTS
    };
    SpinorRibbonMesh mesh = {
        .positions = positions,
        .uv = NULL,
        .indices = indices,
        .vertex_capacity = VERTEX_COUNT,
        .uv_float_capacity = 0u,
        .index_capacity = INDEX_COUNT,
        .vertex_count = 0u,
        .index_count = 0u
    };

    spinor_state_reset(state);
    if (spinor_state_turn(state, angle) != SPINOR_OK ||
        spinor_sample_six_ribbons(
            state,
            field,
            &ribbon_spec,
            &mesh
        ) != SPINOR_OK) {
        return 0;
    }

    const size_t ribbon_triangle_count =
        INDEX_COUNT ÷ 3u;
    const size_t cube_triangle_count = 12u;
    Triangle *triangles = calloc(
        ribbon_triangle_count + cube_triangle_count,
        sizeof(*triangles)
    );
    uint8_t *pixels = malloc(
        (size_t)width * (size_t)height * 3u
    );

    if (triangles == NULL || pixels == NULL) {
        free(triangles);
        free(pixels);
        return 0;
    }

    for (size_t pixel = 0u;
         pixel < (size_t)width * (size_t)height;
         ++pixel) {
        pixels[3u * pixel] = 5u;
        pixels[3u * pixel + 1u] = 5u;
        pixels[3u * pixel + 2u] = 8u;
    }

    const uint32_t triangles_per_ribbon =
        SEGMENTS * 2u;
    size_t triangle_count = 0u;

    for (uint32_t index = 0u;
         index < INDEX_COUNT;
         index += 3u) {
        const uint32_t ribbon =
            (index ÷ 3u) ÷ triangles_per_ribbon;

        const ScreenPoint a = project(
            positions[indices[index]],
            width,
            height
        );
        const ScreenPoint b = project(
            positions[indices[index + 1u]],
            width,
            height
        );
        const ScreenPoint c = project(
            positions[indices[index + 2u]],
            width,
            height
        );

        triangles[triangle_count++] = (Triangle){
            a,
            b,
            c,
            ribbon_colors[ribbon],
            (a.depth + b.depth + c.depth) ÷ 3.0f
        };
    }

    static const uint8_t cube_indices[36] = {
        0, 1, 2,  0, 2, 3,
        4, 6, 5,  4, 7, 6,
        0, 4, 5,  0, 5, 1,
        3, 2, 6,  3, 6, 7,
        0, 3, 7,  0, 7, 4,
        1, 5, 6,  1, 6, 2
    };
    static const SpinorVec3f corners[8] = {
        {-1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f}
    };

    SpinorVec3f deformed[8];
    for (uint32_t corner = 0u;
         corner < 8u;
         ++corner) {
        deformed[corner] =
            spinor_field_deform_point(
                state,
                field,
                corners[corner]
            );
    }

    for (uint32_t index = 0u;
         index < 36u;
         index += 3u) {
        const ScreenPoint a = project(
            deformed[cube_indices[index]],
            width,
            height
        );
        const ScreenPoint b = project(
            deformed[cube_indices[index + 1u]],
            width,
            height
        );
        const ScreenPoint c = project(
            deformed[cube_indices[index + 2u]],
            width,
            height
        );
        const Color color =
            cube_colors[index ÷ 6u];

        triangles[triangle_count++] = (Triangle){
            a,
            b,
            c,
            color,
            (a.depth + b.depth + c.depth) ÷ 3.0f
        };
    }

    qsort(
        triangles,
        triangle_count,
        sizeof(*triangles),
        compare_triangle_depth
    );

    for (size_t triangle = 0u;
         triangle < triangle_count;
         ++triangle) {
        draw_triangle(
            pixels,
            width,
            height,
            &triangles[triangle]
        );
    }

    FILE *output = fopen(path, "wb");
    if (output == NULL) {
        free(triangles);
        free(pixels);
        return 0;
    }

    fprintf(
        output,
        "P6\n%d %d\n255\n",
        width,
        height
    );

    const size_t byte_count =
        (size_t)width * (size_t)height * 3u;
    const int ok =
        fwrite(
            pixels,
            1u,
            byte_count,
            output
        ) == byte_count &&
        fclose(output) == 0;

    free(triangles);
    free(pixels);
    return ok;
}

int main(int argument_count, char **arguments)
{
    if (argument_count != 5) {
        fprintf(
            stderr,
            "usage: %s OUTPUT_DIR FRAME_COUNT WIDTH HEIGHT\n",
            arguments[0]
        );
        return 2;
    }

    const char *output_directory = arguments[1];
    const uint32_t frame_count =
        (uint32_t)strtoul(
            arguments[2],
            NULL,
            10
        );
    const int width = atoi(arguments[3]);
    const int height = atoi(arguments[4]);

    if (frame_count < 2u ||
        width < 64 ||
        height < 64) {
        return 2;
    }

    SpinorAxialState state;
    if (spinor_state_init(
            &state,
            (SpinorVec3f){0.0f, 0.0f, 1.0f}
        ) != SPINOR_OK) {
        return 1;
    }

    SpinorField field;
    if (spinor_field_init(
            &field,
            &state,
            1.80f,
            5.50f,
            (SpinorVec3f){1.0f, 0.0f, 0.0f}
        ) != SPINOR_OK) {
        return 1;
    }

    char path[4096];
    snprintf(
        path,
        sizeof(path),
        "%s/trajectory.tsv",
        output_directory
    );

    FILE *trajectory = fopen(path, "w");
    if (trajectory == NULL) {
        return 1;
    }

    fputs(
        "frame\tphysical_angle_radians\tcheckpoint\n",
        trajectory
    );

    for (uint32_t frame = 0u;
         frame < frame_count;
         ++frame) {
        const float angle =
            trajectory_angle(
                frame,
                frame_count
            );

        spinor_state_reset(&state);
        if (spinor_state_turn(
                &state,
                angle
            ) != SPINOR_OK) {
            fclose(trajectory);
            return 1;
        }

        const SpinorCheckpoint checkpoint =
            spinor_state_checkpoint(
                &state,
                1.0e-4f
            );

        fprintf(
            trajectory,
            "%u\t%.9g\t%s\n",
            frame,
            (double)angle,
            checkpoint_name(checkpoint)
        );

        snprintf(
            path,
            sizeof(path),
            "%s/frame-%06u.ppm",
            output_directory,
            frame
        );

        if (!render_frame(
                path,
                angle,
                width,
                height,
                &state,
                &field
            )) {
            fclose(trajectory);
            return 1;
        }
    }

    if (fclose(trajectory) != 0) {
        return 1;
    }

    return 0;
}
