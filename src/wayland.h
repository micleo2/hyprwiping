#pragma once

#include <stdbool.h>

extern "C" {
#include <wayland-client.h>
#include <wayland-egl.h>
// generated header uses "namespace" as a param name — valid C, keyword in C++
#define namespace namespace_
#include "wlr-layer-shell-protocol.h"
#undef namespace
}

struct wl_state {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct wl_egl_window *egl_window;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct zwlr_layer_surface_v1 *layer_surface;
    struct wl_seat *seat;
    struct wl_pointer *pointer;
    struct wl_keyboard *keyboard;
    struct wl_output *output;

    uint32_t width;
    uint32_t height;
    int32_t scale;
    bool configured;
    bool running;
};

bool wayland_init(struct wl_state *state);
void wayland_cleanup(struct wl_state *state);
