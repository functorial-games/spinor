#ifndef SPINOR_RENDERER_H
#define SPINOR_RENDERER_H

#include "../native/spinor_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Android owns EGL and calls this interface only while its context is current.
 * The renderer owns GLES resources and consumes the application semantic core.
 * No Activity, JNI, ANativeWindow or DEX type crosses this boundary.
 */

int spinor_renderer_start(int width, int height, int gles_major);
void spinor_renderer_resize(int width, int height);

int spinor_renderer_turn_body(float delta_radians);
int spinor_renderer_set_physical_angle(float physical_angle);
float spinor_renderer_physical_angle(void);
SpinorCheckpoint spinor_renderer_checkpoint(float angular_tolerance);

void spinor_renderer_draw(void);
void spinor_renderer_stop(void);

#ifdef __cplusplus
}
#endif

#endif
