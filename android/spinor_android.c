#include <EGL/egl.h>
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#include <android_native_app_glue.h>
#pragma clang diagnostic pop

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "spinor_renderer.h"

#define SPINOR_LOG_TAG "SpinorNative"
#define SPINOR_LOG(...) __android_log_print(ANDROID_LOG_INFO, SPINOR_LOG_TAG, __VA_ARGS__)
#define SPINOR_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, SPINOR_LOG_TAG, __VA_ARGS__)

#define SPINOR_TAU_F 6.28318530717958647692f

typedef struct {
    float physical_angle;
} SpinorSavedState;

typedef struct {
    struct android_app *app;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

    int width;
    int height;
    bool renderer_started;
    bool redraw;
    bool focused;

    int32_t active_pointer_id;
    float previous_x;
} SpinorAndroidState;

static void release_grab(SpinorAndroidState *state)
{
    state->active_pointer_id = -1;
}

static int short_side(const SpinorAndroidState *state)
{
    if (state->width <= 0 || state->height <= 0) {
        return 1;
    }
    return state->width < state->height ? state->width : state->height;
}

static bool body_hit(
    const SpinorAndroidState *state,
    float x,
    float y
)
{
    const float center_x = 0.5f * (float)state->width;
    const float center_y = 0.5f * (float)state->height;
    const float dx = x - center_x;
    const float dy = y - center_y;
    const float radius = 0.22f * (float)short_side(state);
    return dx * dx + dy * dy <= radius * radius;
}

static int pointer_index_for_id(
    AInputEvent *event,
    int32_t pointer_id
)
{
    const size_t count = AMotionEvent_getPointerCount(event);
    for (size_t index = 0u; index < count; ++index) {
        if (AMotionEvent_getPointerId(event, index) == pointer_id) {
            return (int)index;
        }
    }
    return -1;
}

static void stop_surface(SpinorAndroidState *state)
{
    if (state->renderer_started) {
        spinor_renderer_stop();
        state->renderer_started = false;
    }

    if (state->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(
            state->display,
            EGL_NO_SURFACE,
            EGL_NO_SURFACE,
            EGL_NO_CONTEXT
        );

        if (state->context != EGL_NO_CONTEXT) {
            eglDestroyContext(state->display, state->context);
        }
        if (state->surface != EGL_NO_SURFACE) {
            eglDestroySurface(state->display, state->surface);
        }

        eglTerminate(state->display);
    }

    state->display = EGL_NO_DISPLAY;
    state->surface = EGL_NO_SURFACE;
    state->context = EGL_NO_CONTEXT;
    state->width = 0;
    state->height = 0;
    state->redraw = false;
    release_grab(state);
}

static bool start_surface(SpinorAndroidState *state)
{
    if (state->app->window == NULL) {
        return false;
    }

    const float retained_angle = spinor_renderer_physical_angle();
    stop_surface(state);
    (void)spinor_renderer_set_physical_angle(retained_angle);

    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (state->display == EGL_NO_DISPLAY) {
        return false;
    }

    if (eglInitialize(state->display, NULL, NULL) != EGL_TRUE) {
        stop_surface(state);
        return false;
    }

    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
        stop_surface(state);
        return false;
    }

    const EGLint config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };

    EGLConfig config = NULL;
    EGLint config_count = 0;
    if (eglChooseConfig(
            state->display,
            config_attributes,
            &config,
            1,
            &config_count
        ) != EGL_TRUE || config_count != 1) {
        stop_surface(state);
        return false;
    }

    EGLint native_format = 0;
    if (eglGetConfigAttrib(
            state->display,
            config,
            EGL_NATIVE_VISUAL_ID,
            &native_format
        ) != EGL_TRUE) {
        stop_surface(state);
        return false;
    }

    ANativeWindow_setBuffersGeometry(
        state->app->window,
        0,
        0,
        native_format
    );

    const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    state->context = eglCreateContext(
        state->display,
        config,
        EGL_NO_CONTEXT,
        context_attributes
    );
    if (state->context == EGL_NO_CONTEXT) {
        stop_surface(state);
        return false;
    }

    state->surface = eglCreateWindowSurface(
        state->display,
        config,
        state->app->window,
        NULL
    );
    if (state->surface == EGL_NO_SURFACE) {
        stop_surface(state);
        return false;
    }

    if (eglMakeCurrent(
            state->display,
            state->surface,
            state->surface,
            state->context
        ) != EGL_TRUE) {
        stop_surface(state);
        return false;
    }

    EGLint width = 0;
    EGLint height = 0;
    eglQuerySurface(state->display, state->surface, EGL_WIDTH, &width);
    eglQuerySurface(state->display, state->surface, EGL_HEIGHT, &height);

    if (width <= 0 || height <= 0 ||
        !spinor_renderer_start(width, height, 2)) {
        stop_surface(state);
        return false;
    }

    state->width = width;
    state->height = height;
    state->renderer_started = true;
    state->redraw = true;

    (void)eglSwapInterval(state->display, 1);

    SPINOR_LOG("EGL ready: %dx%d GLES2", width, height);
    return true;
}

static void resize_surface(SpinorAndroidState *state)
{
    if (!state->renderer_started || state->surface == EGL_NO_SURFACE) {
        return;
    }

    EGLint width = 0;
    EGLint height = 0;
    eglQuerySurface(state->display, state->surface, EGL_WIDTH, &width);
    eglQuerySurface(state->display, state->surface, EGL_HEIGHT, &height);

    if (width > 0 && height > 0) {
        state->width = width;
        state->height = height;
        spinor_renderer_resize(width, height);
        state->redraw = true;
        release_grab(state);
    }
}

static void save_state(SpinorAndroidState *state)
{
    SpinorSavedState *saved = malloc(sizeof(*saved));
    if (saved == NULL) {
        return;
    }

    saved->physical_angle = spinor_renderer_physical_angle();
    state->app->savedState = saved;
    state->app->savedStateSize = sizeof(*saved);
}

static void handle_command(
    struct android_app *app,
    int32_t command
)
{
    SpinorAndroidState *state =
        (SpinorAndroidState *)app->userData;

    switch (command) {
        case APP_CMD_INIT_WINDOW:
            (void)start_surface(state);
            break;

        case APP_CMD_TERM_WINDOW:
            stop_surface(state);
            break;

        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONFIG_CHANGED:
        case APP_CMD_CONTENT_RECT_CHANGED:
            resize_surface(state);
            break;

        case APP_CMD_GAINED_FOCUS:
        case APP_CMD_RESUME:
            state->focused = true;
            state->redraw = true;
            break;

        case APP_CMD_LOST_FOCUS:
        case APP_CMD_PAUSE:
            state->focused = false;
            release_grab(state);
            break;

        case APP_CMD_SAVE_STATE:
            save_state(state);
            break;

        default:
            break;
    }
}

static int32_t handle_input(
    struct android_app *app,
    AInputEvent *event
)
{
    SpinorAndroidState *state =
        (SpinorAndroidState *)app->userData;

    if (!state->focused ||
        !state->renderer_started ||
        AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    const int32_t action = AMotionEvent_getAction(event);
    const int32_t masked = action & AMOTION_EVENT_ACTION_MASK;
    const int32_t pointer_index =
        (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
        AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    if (masked == AMOTION_EVENT_ACTION_CANCEL) {
        release_grab(state);
        return 1;
    }

    if (masked == AMOTION_EVENT_ACTION_DOWN ||
        masked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        if (state->active_pointer_id >= 0) {
            return 1;
        }

        const size_t count = AMotionEvent_getPointerCount(event);
        if (pointer_index < 0 ||
            (size_t)pointer_index >= count) {
            return 1;
        }

        const float x =
            AMotionEvent_getX(event, (size_t)pointer_index);
        const float y =
            AMotionEvent_getY(event, (size_t)pointer_index);

        if (!body_hit(state, x, y)) {
            return 0;
        }

        state->active_pointer_id =
            AMotionEvent_getPointerId(
                event,
                (size_t)pointer_index
            );
        state->previous_x = x;
        return 1;
    }

    if (masked == AMOTION_EVENT_ACTION_MOVE &&
        state->active_pointer_id >= 0) {
        const int index =
            pointer_index_for_id(
                event,
                state->active_pointer_id
            );
        if (index < 0) {
            release_grab(state);
            return 1;
        }

        const float x =
            AMotionEvent_getX(event, (size_t)index);
        const float delta_x = x - state->previous_x;
        state->previous_x = x;

        const float delta_angle =
            SPINOR_TAU_F *
            delta_x /
            (float)short_side(state);

        if (fabsf(delta_angle) > 0.0f &&
            spinor_renderer_turn_body(delta_angle)) {
            state->redraw = true;
        }

        return 1;
    }

    if ((masked == AMOTION_EVENT_ACTION_UP ||
         masked == AMOTION_EVENT_ACTION_POINTER_UP) &&
        state->active_pointer_id >= 0) {
        const size_t count = AMotionEvent_getPointerCount(event);
        if (pointer_index >= 0 &&
            (size_t)pointer_index < count) {
            const int32_t released_id =
                AMotionEvent_getPointerId(
                    event,
                    (size_t)pointer_index
                );
            if (released_id == state->active_pointer_id) {
                release_grab(state);
            }
        }
        return 1;
    }

    return 0;
}

void android_main(struct android_app *app)
{
    SPINOR_LOG("native entry");

    SpinorAndroidState state;
    memset(&state, 0, sizeof(state));

    state.app = app;
    state.display = EGL_NO_DISPLAY;
    state.surface = EGL_NO_SURFACE;
    state.context = EGL_NO_CONTEXT;
    state.active_pointer_id = -1;
    state.focused = true;

    app->userData = &state;
    app->onAppCmd = handle_command;
    app->onInputEvent = handle_input;

    if (app->savedState != NULL &&
        app->savedStateSize == sizeof(SpinorSavedState)) {
        const SpinorSavedState *saved =
            (const SpinorSavedState *)app->savedState;
        (void)spinor_renderer_set_physical_angle(
            saved->physical_angle
        );
    }

    for (;;) {
        int events = 0;
        struct android_poll_source *source = NULL;
        const int timeout =
            state.renderer_started && state.redraw ? 0 : -1;

        int ident = 0;
        while ((ident = ALooper_pollOnce(
                    timeout,
                    NULL,
                    &events,
                    (void **)&source
                )) >= 0) {
            (void)ident;

            if (source != NULL) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                stop_surface(&state);
                return;
            }

            if (state.renderer_started && state.redraw) {
                break;
            }
        }

        if (app->destroyRequested != 0) {
            stop_surface(&state);
            return;
        }

        if (state.renderer_started && state.redraw) {
            spinor_renderer_draw();

            if (eglSwapBuffers(
                    state.display,
                    state.surface
                ) != EGL_TRUE) {
                SPINOR_LOGE(
                    "eglSwapBuffers failed: 0x%x",
                    eglGetError()
                );
                stop_surface(&state);
                continue;
            }

            state.redraw = false;
        }
    }
}
