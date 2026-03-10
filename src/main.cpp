#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include "wayland.h"
#include "render.h"

static struct wl_state wl;
static struct render_state rs;

static void handle_signal(int sig) {
    (void)sig;
    wl.running = false;
}

static void usage(const char *name) {
    fprintf(stderr,
        "Usage: %s --shader <frag> --screenshot <image> [--display <frag>]\n",
        name);
}

static void frame_done(void *data, struct wl_callback *callback,
        uint32_t time);

static struct wl_callback_listener frame_listener = {
    .done = frame_done,
};

static void frame_done(void *data, struct wl_callback *callback,
        uint32_t time) {
    (void)time;
    wl_callback_destroy(callback);

    auto *state = (struct wl_state *)data;
    if (!state->running) return;

    struct wl_callback *cb = wl_surface_frame(state->surface);
    wl_callback_add_listener(cb, &frame_listener, state);
    render_frame(&rs, state);
}

int main(int argc, char *argv[]) {
    const char *shader_path = nullptr;
    const char *screenshot_path = nullptr;
    const char *display_path = nullptr;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--shader") == 0 && i + 1 < argc) {
            shader_path = argv[++i];
        } else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            screenshot_path = argv[++i];
        } else if (strcmp(argv[i], "--display") == 0 && i + 1 < argc) {
            display_path = argv[++i];
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (!shader_path || !screenshot_path) {
        usage(argv[0]);
        return 1;
    }

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    if (!wayland_init(&wl)) return 1;
    if (!render_init(&rs, &wl, screenshot_path, shader_path, display_path)) {
        wayland_cleanup(&wl);
        return 1;
    }

    // kick off the callback chain, then render first frame
    struct wl_callback *cb = wl_surface_frame(wl.surface);
    wl_callback_add_listener(cb, &frame_listener, &wl);
    render_frame(&rs, &wl);

    while (wl.running && wl_display_dispatch(wl.display) != -1) {
        // frame callbacks drive rendering
    }

    render_cleanup(&rs);
    wayland_cleanup(&wl);
    return 0;
}
