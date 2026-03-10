#include "render.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#define STB_IMAGE_IMPLEMENTATION
extern "C" {
#include "stb_image.h"
}

static const char *vert_src = R"(
    attribute vec2 a_pos;
    varying vec2 v_uv;
    void main() {
        v_uv = a_pos * 0.5 + 0.5;
        gl_Position = vec4(a_pos, 0.0, 1.0);
    }
)";

static const char *passthrough_frag_src = R"(
    precision mediump float;
    varying vec2 v_uv;
    uniform sampler2D u_texture;
    void main() {
        gl_FragColor = texture2D(u_texture, v_uv);
    }
)";

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Failed to open shader: %s\n", path);
        return nullptr;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        fprintf(stderr, "Shader compile error:\n%s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint link_program(const char *vert, const char *frag) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vert);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag);
    if (!vs || !fs) return 0;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "a_pos");
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        fprintf(stderr, "Program link error:\n%s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

static bool init_egl(struct render_state *rs, struct wl_state *wl) {
    rs->egl_display = eglGetDisplay((EGLNativeDisplayType)wl->display);
    if (rs->egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Failed to get EGL display\n");
        return false;
    }
    if (!eglInitialize(rs->egl_display, nullptr, nullptr)) {
        fprintf(stderr, "Failed to initialize EGL\n");
        return false;
    }
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE,
    };
    EGLConfig config;
    EGLint num;
    eglChooseConfig(rs->egl_display, attribs, &config, 1, &num);

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    rs->egl_context = eglCreateContext(rs->egl_display, config,
        EGL_NO_CONTEXT, ctx_attribs);

    rs->egl_surface = eglCreateWindowSurface(rs->egl_display, config,
        (EGLNativeWindowType)wl->egl_window, nullptr);

    eglMakeCurrent(rs->egl_display, rs->egl_surface,
        rs->egl_surface, rs->egl_context);
    return true;
}

static bool init_fbos(struct render_state *rs, struct wl_state *wl,
                      const char *screenshot_path) {
    int img_w, img_h, img_ch;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *pixels = stbi_load(screenshot_path, &img_w, &img_h,
        &img_ch, 4);
    if (!pixels) {
        fprintf(stderr, "Failed to load screenshot: %s\n", screenshot_path);
        return false;
    }

    // keep a separate copy of the screenshot that never gets written to
    glGenTextures(1, &rs->screenshot_tex);
    glBindTexture(GL_TEXTURE_2D, rs->screenshot_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_w, img_h, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    for (int i = 0; i < 2; i++) {
        glGenTextures(1, &rs->fbo_tex[i]);
        glBindTexture(GL_TEXTURE_2D, rs->fbo_tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if (i == 0) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_w, img_h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wl->width, wl->height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }

        glGenFramebuffers(1, &rs->fbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, rs->fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, rs->fbo_tex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "Framebuffer %d incomplete\n", i);
            stbi_image_free(pixels);
            return false;
        }
    }

    stbi_image_free(pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    rs->read_idx = 0;
    return true;
}

static void init_quad(struct render_state *rs) {
    float verts[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
    };
    glGenBuffers(1, &rs->quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rs->quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
}

static void draw_quad() {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(0);
}

static double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

bool render_init(struct render_state *rs, struct wl_state *wl,
                 const char *screenshot_path, const char *shader_path,
                 const char *display_path) {
    memset(rs, 0, sizeof(*rs));

    if (!init_egl(rs, wl)) return false;

    char *frag_src = read_file(shader_path);
    if (!frag_src) return false;

    rs->feedback_program = link_program(vert_src, frag_src);
    free(frag_src);
    if (!rs->feedback_program) return false;

    // display shader: load from file or fall back to passthrough
    if (display_path) {
        char *disp_src = read_file(display_path);
        if (!disp_src) return false;
        rs->display_program = link_program(vert_src, disp_src);
        free(disp_src);
    } else {
        rs->display_program = link_program(vert_src, passthrough_frag_src);
    }
    if (!rs->display_program) return false;

    rs->fb_u_texture = glGetUniformLocation(rs->feedback_program, "u_texture");
    rs->fb_u_screenshot = glGetUniformLocation(rs->feedback_program, "u_screenshot");
    rs->fb_u_time = glGetUniformLocation(rs->feedback_program, "u_time");
    rs->fb_u_frame = glGetUniformLocation(rs->feedback_program, "u_frame");
    rs->fb_u_resolution = glGetUniformLocation(rs->feedback_program, "u_resolution");

    rs->disp_u_texture = glGetUniformLocation(rs->display_program, "u_texture");
    rs->disp_u_screenshot = glGetUniformLocation(rs->display_program, "u_screenshot");
    rs->disp_u_time = glGetUniformLocation(rs->display_program, "u_time");
    rs->disp_u_frame = glGetUniformLocation(rs->display_program, "u_frame");
    rs->disp_u_resolution = glGetUniformLocation(rs->display_program, "u_resolution");

    if (!init_fbos(rs, wl, screenshot_path)) return false;
    init_quad(rs);

    rs->start_time = get_time_sec();
    rs->frame_count = 0;

    return true;
}

void render_frame(struct render_state *rs, struct wl_state *wl) {
    int write_idx = 1 - rs->read_idx;
    float now = (float)(get_time_sec() - rs->start_time);

    glBindBuffer(GL_ARRAY_BUFFER, rs->quad_vbo);

    // feedback pass: read -> write FBO
    glBindFramebuffer(GL_FRAMEBUFFER, rs->fbo[write_idx]);
    glViewport(0, 0, wl->width, wl->height);
    glUseProgram(rs->feedback_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rs->fbo_tex[rs->read_idx]);
    glUniform1i(rs->fb_u_texture, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rs->screenshot_tex);
    glUniform1i(rs->fb_u_screenshot, 1);
    glUniform1f(rs->fb_u_time, now);
    glUniform1i(rs->fb_u_frame, rs->frame_count);
    glUniform2f(rs->fb_u_resolution, (float)wl->width, (float)wl->height);
    draw_quad();

    // display pass: write FBO -> screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, wl->width, wl->height);
    glUseProgram(rs->display_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rs->fbo_tex[write_idx]);
    glUniform1i(rs->disp_u_texture, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rs->screenshot_tex);
    glUniform1i(rs->disp_u_screenshot, 1);
    glUniform1f(rs->disp_u_time, now);
    glUniform1i(rs->disp_u_frame, rs->frame_count);
    glUniform2f(rs->disp_u_resolution, (float)wl->width, (float)wl->height);
    draw_quad();

    eglSwapBuffers(rs->egl_display, rs->egl_surface);

    rs->read_idx = write_idx;
    rs->frame_count++;
}

void render_cleanup(struct render_state *rs) {
    glDeleteBuffers(1, &rs->quad_vbo);
    glDeleteFramebuffers(2, rs->fbo);
    glDeleteTextures(2, rs->fbo_tex);
    glDeleteTextures(1, &rs->screenshot_tex);
    glDeleteProgram(rs->feedback_program);
    glDeleteProgram(rs->display_program);
    if (rs->egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(rs->egl_display, EGL_NO_SURFACE,
            EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (rs->egl_surface != EGL_NO_SURFACE)
            eglDestroySurface(rs->egl_display, rs->egl_surface);
        if (rs->egl_context != EGL_NO_CONTEXT)
            eglDestroyContext(rs->egl_display, rs->egl_context);
        eglTerminate(rs->egl_display);
    }
}
