#pragma once

extern "C" {
#include <EGL/egl.h>
#include <GLES2/gl2.h>
}
#include "wayland.h"

struct render_state {
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;

    GLuint fbo[2];
    GLuint fbo_tex[2];
    GLuint screenshot_tex;
    int read_idx;

    GLuint feedback_program;
    GLuint display_program;
    GLuint quad_vbo;

    GLint fb_u_texture;
    GLint fb_u_screenshot;
    GLint fb_u_time;
    GLint fb_u_frame;
    GLint fb_u_resolution;

    GLint disp_u_texture;
    GLint disp_u_screenshot;
    GLint disp_u_time;
    GLint disp_u_frame;
    GLint disp_u_resolution;

    uint32_t frame_count;
    double start_time;
};

bool render_init(struct render_state *rs, struct wl_state *wl,
                 const char *screenshot_path, const char *shader_path,
                 const char *display_path);
void render_frame(struct render_state *rs, struct wl_state *wl);
void render_cleanup(struct render_state *rs);
