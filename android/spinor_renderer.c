#include "spinor_renderer.h"

#include <GLES2/gl2.h>
#include <android/log.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../native/spinor_field.h"
#include "../native/spinor_ribbons.h"

#define SPINOR_LOG_TAG "SpinorRenderer"
#define SPINOR_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, SPINOR_LOG_TAG, __VA_ARGS__)
#define SPINOR_LOGI(...) __android_log_print(ANDROID_LOG_INFO, SPINOR_LOG_TAG, __VA_ARGS__)

#define SPINOR_RENDER_SEGMENTS 48u
#define SPINOR_RENDER_VERTEX_COUNT \
    (SPINOR_RIBBON_COUNT * (SPINOR_RENDER_SEGMENTS + 1u) * 2u)
#define SPINOR_RENDER_INDEX_COUNT \
    (SPINOR_RIBBON_COUNT * SPINOR_RENDER_SEGMENTS * 6u)
#define SPINOR_CUBE_TRIANGLE_VERTEX_COUNT 36u

typedef struct {
    GLfloat position[3];
    GLfloat color[3];
} SpinorRenderVertex;

static const char *SPINOR_VERTEX_SHADER =
    "attribute vec3 a_position;\n"
    "attribute vec3 a_color;\n"
    "uniform float u_aspect;\n"
    "varying vec3 v_color;\n"
    "void main() {\n"
    "  const float cy = 0.8525245221;\n"
    "  const float sy = 0.5226872289;\n"
    "  const float cp = 0.9004471024;\n"
    "  const float sp = -0.4349655341;\n"
    "  vec3 p = a_position;\n"
    "  vec3 q;\n"
    "  q.x = cy * p.x + sy * p.z;\n"
    "  q.y = p.y;\n"
    "  q.z = -sy * p.x + cy * p.z;\n"
    "  vec3 v;\n"
    "  v.x = q.x;\n"
    "  v.y = cp * q.y - sp * q.z;\n"
    "  v.z = sp * q.y + cp * q.z - 18.0;\n"
    "  const float focal = 1.15;\n"
    "  const float depth_a = -1.0512820513;\n"
    "  const float depth_b = -2.0512820513;\n"
    "  gl_Position = vec4(\n"
    "    focal * v.x / max(u_aspect, 0.01),\n"
    "    focal * v.y,\n"
    "    depth_a * v.z + depth_b,\n"
    "    -v.z\n"
    "  );\n"
    "  v_color = a_color;\n"
    "}\n";

static const char *SPINOR_FRAGMENT_SHADER =
    "precision mediump float;\n"
    "varying vec3 v_color;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(v_color, 1.0);\n"
    "}\n";

static bool model_ready = false;
static bool gl_ready = false;
static bool geometry_dirty = true;

static int render_width = 1;
static int render_height = 1;

static GLuint program = 0u;
static GLuint ribbon_vbo = 0u;
static GLuint ribbon_ebo = 0u;
static GLuint cube_vbo = 0u;
static GLint position_location = -1;
static GLint color_location = -1;
static GLint aspect_location = -1;

static SpinorAxialState spin_state;
static SpinorField field;
static SpinorRibbonSpec ribbon_spec;

static SpinorVec3f sampled_positions[SPINOR_RENDER_VERTEX_COUNT];
static uint32_t sampled_indices[SPINOR_RENDER_INDEX_COUNT];
static uint16_t draw_indices[SPINOR_RENDER_INDEX_COUNT];
static SpinorRenderVertex ribbon_vertices[SPINOR_RENDER_VERTEX_COUNT];
static SpinorRenderVertex cube_vertices[SPINOR_CUBE_TRIANGLE_VERTEX_COUNT];

static const GLfloat RIBBON_COLORS[SPINOR_RIBBON_COUNT][3] = {
    {0.92f, 0.26f, 0.22f},
    {0.66f, 0.12f, 0.12f},
    {0.30f, 0.86f, 0.38f},
    {0.10f, 0.58f, 0.18f},
    {0.26f, 0.48f, 0.98f},
    {0.12f, 0.24f, 0.72f}
};

static GLuint compile_shader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    if (shader == 0u) {
        return 0u;
    }

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        char log[1024] = {0};
        GLsizei length = 0;
        glGetShaderInfoLog(shader, (GLsizei)sizeof(log), &length, log);
        SPINOR_LOGE("shader compile failed: %s", log);
        glDeleteShader(shader);
        return 0u;
    }

    return shader;
}

static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint linked_program = glCreateProgram();
    if (linked_program == 0u) {
        return 0u;
    }

    glAttachShader(linked_program, vertex_shader);
    glAttachShader(linked_program, fragment_shader);
    glBindAttribLocation(linked_program, 0u, "a_position");
    glBindAttribLocation(linked_program, 1u, "a_color");
    glLinkProgram(linked_program);

    GLint linked = GL_FALSE;
    glGetProgramiv(linked_program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char log[1024] = {0};
        GLsizei length = 0;
        glGetProgramInfoLog(linked_program, (GLsizei)sizeof(log), &length, log);
        SPINOR_LOGE("program link failed: %s", log);
        glDeleteProgram(linked_program);
        return 0u;
    }

    return linked_program;
}

static int ensure_model(void)
{
    if (model_ready) {
        return 1;
    }

    if (spinor_state_init(
            &spin_state,
            (SpinorVec3f){0.0f, 0.0f, 1.0f}
        ) != SPINOR_OK) {
        return 0;
    }

    /*
     * The rigid core contains the whole cube (corner radius sqrt(3)).
     * The exterior shell is the six-ribbon fixed-anchor radius.
     */
    if (spinor_field_init(
            &field,
            &spin_state,
            1.80f,
            5.50f,
            (SpinorVec3f){1.0f, 0.0f, 0.0f}
        ) != SPINOR_OK) {
        return 0;
    }

    ribbon_spec.body_half_extent = 1.0f;
    ribbon_spec.width = 0.28f;
    ribbon_spec.segments = SPINOR_RENDER_SEGMENTS;

    model_ready = true;
    geometry_dirty = true;
    return 1;
}

static void set_vertex(
    SpinorRenderVertex *vertex,
    SpinorVec3f position,
    const GLfloat color[3]
)
{
    vertex->position[0] = position.x;
    vertex->position[1] = position.y;
    vertex->position[2] = position.z;
    vertex->color[0] = color[0];
    vertex->color[1] = color[1];
    vertex->color[2] = color[2];
}

static int rebuild_ribbons(void)
{
    SpinorRibbonMesh mesh = {
        .positions = sampled_positions,
        .uv = NULL,
        .indices = sampled_indices,
        .vertex_capacity = SPINOR_RENDER_VERTEX_COUNT,
        .uv_float_capacity = 0u,
        .index_capacity = SPINOR_RENDER_INDEX_COUNT,
        .vertex_count = 0u,
        .index_count = 0u
    };

    if (spinor_sample_six_ribbons(
            &spin_state,
            &field,
            &ribbon_spec,
            &mesh
        ) != SPINOR_OK) {
        SPINOR_LOGE("six-ribbon sample failed");
        return 0;
    }

    if (mesh.vertex_count != SPINOR_RENDER_VERTEX_COUNT ||
        mesh.index_count != SPINOR_RENDER_INDEX_COUNT) {
        SPINOR_LOGE(
            "unexpected ribbon mesh size: vertices=%u indices=%u",
            mesh.vertex_count,
            mesh.index_count
        );
        return 0;
    }

    const uint32_t vertices_per_ribbon =
        (SPINOR_RENDER_SEGMENTS + 1u) * 2u;

    for (uint32_t vertex = 0u; vertex < mesh.vertex_count; ++vertex) {
        const uint32_t ribbon = vertex / vertices_per_ribbon;
        set_vertex(
            &ribbon_vertices[vertex],
            sampled_positions[vertex],
            RIBBON_COLORS[ribbon]
        );
    }

    for (uint32_t index = 0u; index < mesh.index_count; ++index) {
        if (sampled_indices[index] > UINT16_MAX) {
            SPINOR_LOGE("ribbon index exceeds GLES2 uint16 range");
            return 0;
        }
        draw_indices[index] = (uint16_t)sampled_indices[index];
    }

    return 1;
}

static int rebuild_cube(void)
{
    static const uint8_t triangle_indices[SPINOR_CUBE_TRIANGLE_VERTEX_COUNT] = {
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

    static const GLfloat face_colors[6][3] = {
        {0.96f, 0.87f, 0.55f},
        {0.82f, 0.70f, 0.36f},
        {0.92f, 0.81f, 0.48f},
        {0.72f, 0.61f, 0.32f},
        {0.88f, 0.76f, 0.42f},
        {0.78f, 0.66f, 0.34f}
    };

    for (uint32_t vertex = 0u;
         vertex < SPINOR_CUBE_TRIANGLE_VERTEX_COUNT;
         ++vertex) {
        const uint32_t face = vertex / 6u;
        const SpinorVec3f reference =
            corners[triangle_indices[vertex]];
        const SpinorVec3f deformed =
            spinor_field_deform_point(&spin_state, &field, reference);

        set_vertex(
            &cube_vertices[vertex],
            deformed,
            face_colors[face]
        );
    }

    return 1;
}

static int upload_geometry(void)
{
    if (!rebuild_ribbons() || !rebuild_cube()) {
        return 0;
    }

    glBindBuffer(GL_ARRAY_BUFFER, ribbon_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        (GLsizeiptr)sizeof(ribbon_vertices),
        ribbon_vertices,
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ribbon_ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        (GLsizeiptr)sizeof(draw_indices),
        draw_indices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        (GLsizeiptr)sizeof(cube_vertices),
        cube_vertices,
        GL_DYNAMIC_DRAW
    );

    geometry_dirty = false;
    return glGetError() == GL_NO_ERROR;
}

int spinor_renderer_start(int width, int height, int gles_major)
{
    if (gles_major < 2 || width <= 0 || height <= 0 || !ensure_model()) {
        return 0;
    }

    spinor_renderer_stop();

    GLuint vertex_shader = compile_shader(
        GL_VERTEX_SHADER,
        SPINOR_VERTEX_SHADER
    );
    GLuint fragment_shader = compile_shader(
        GL_FRAGMENT_SHADER,
        SPINOR_FRAGMENT_SHADER
    );

    if (vertex_shader == 0u || fragment_shader == 0u) {
        if (vertex_shader != 0u) {
            glDeleteShader(vertex_shader);
        }
        if (fragment_shader != 0u) {
            glDeleteShader(fragment_shader);
        }
        return 0;
    }

    program = link_program(vertex_shader, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    if (program == 0u) {
        return 0;
    }

    position_location = glGetAttribLocation(program, "a_position");
    color_location = glGetAttribLocation(program, "a_color");
    aspect_location = glGetUniformLocation(program, "u_aspect");

    if (position_location < 0 || color_location < 0 || aspect_location < 0) {
        spinor_renderer_stop();
        return 0;
    }

    glGenBuffers(1, &ribbon_vbo);
    glGenBuffers(1, &ribbon_ebo);
    glGenBuffers(1, &cube_vbo);

    if (ribbon_vbo == 0u || ribbon_ebo == 0u || cube_vbo == 0u) {
        spinor_renderer_stop();
        return 0;
    }

    render_width = width;
    render_height = height;
    gl_ready = true;
    geometry_dirty = true;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glClearColor(0.018f, 0.020f, 0.028f, 1.0f);

    if (!upload_geometry()) {
        spinor_renderer_stop();
        return 0;
    }

    SPINOR_LOGI(
        "renderer ready: %dx%d GLES %d, ribbons=%u vertices",
        width,
        height,
        gles_major,
        SPINOR_RENDER_VERTEX_COUNT
    );
    return 1;
}

void spinor_renderer_resize(int width, int height)
{
    if (width > 0 && height > 0) {
        render_width = width;
        render_height = height;
    }
}

int spinor_renderer_turn_body(float delta_radians)
{
    if (!ensure_model()) {
        return 0;
    }

    if (spinor_state_turn(&spin_state, delta_radians) != SPINOR_OK) {
        return 0;
    }

    geometry_dirty = true;
    return 1;
}

int spinor_renderer_set_physical_angle(float physical_angle)
{
    if (!ensure_model() || !isfinite(physical_angle)) {
        return 0;
    }

    spinor_state_reset(&spin_state);
    if (spinor_state_turn(&spin_state, physical_angle) != SPINOR_OK) {
        return 0;
    }

    geometry_dirty = true;
    return 1;
}

float spinor_renderer_physical_angle(void)
{
    if (!ensure_model()) {
        return 0.0f;
    }
    return spin_state.physical_angle;
}

SpinorCheckpoint spinor_renderer_checkpoint(float angular_tolerance)
{
    if (!ensure_model()) {
        return SPINOR_CHECKPOINT_BETWEEN;
    }
    return spinor_state_checkpoint(&spin_state, angular_tolerance);
}

static void bind_vertex_layout(GLuint vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray((GLuint)position_location);
    glVertexAttribPointer(
        (GLuint)position_location,
        3,
        GL_FLOAT,
        GL_FALSE,
        (GLsizei)sizeof(SpinorRenderVertex),
        (const void *)offsetof(SpinorRenderVertex, position)
    );

    glEnableVertexAttribArray((GLuint)color_location);
    glVertexAttribPointer(
        (GLuint)color_location,
        3,
        GL_FLOAT,
        GL_FALSE,
        (GLsizei)sizeof(SpinorRenderVertex),
        (const void *)offsetof(SpinorRenderVertex, color)
    );
}

void spinor_renderer_draw(void)
{
    if (!gl_ready) {
        return;
    }

    if (geometry_dirty && !upload_geometry()) {
        SPINOR_LOGE("geometry upload failed");
        return;
    }

    glViewport(0, 0, render_width, render_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);
    glUniform1f(
        aspect_location,
        (float)render_width / (float)render_height
    );

    bind_vertex_layout(ribbon_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ribbon_ebo);
    glDrawElements(
        GL_TRIANGLES,
        (GLsizei)SPINOR_RENDER_INDEX_COUNT,
        GL_UNSIGNED_SHORT,
        (const void *)0
    );

    bind_vertex_layout(cube_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
    glDrawArrays(
        GL_TRIANGLES,
        0,
        (GLsizei)SPINOR_CUBE_TRIANGLE_VERTEX_COUNT
    );

    glDisableVertexAttribArray((GLuint)position_location);
    glDisableVertexAttribArray((GLuint)color_location);
}

void spinor_renderer_stop(void)
{
    if (ribbon_vbo != 0u) {
        glDeleteBuffers(1, &ribbon_vbo);
    }
    if (ribbon_ebo != 0u) {
        glDeleteBuffers(1, &ribbon_ebo);
    }
    if (cube_vbo != 0u) {
        glDeleteBuffers(1, &cube_vbo);
    }
    if (program != 0u) {
        glDeleteProgram(program);
    }

    ribbon_vbo = 0u;
    ribbon_ebo = 0u;
    cube_vbo = 0u;
    program = 0u;
    position_location = -1;
    color_location = -1;
    aspect_location = -1;
    gl_ready = false;
}
