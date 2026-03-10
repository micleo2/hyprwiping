#include "wayland.h"
#include <cstdio>
#include <cstring>

static void layer_surface_configure(void *data,
        struct zwlr_layer_surface_v1 *surface,
        uint32_t serial, uint32_t width, uint32_t height) {
    auto *state = (struct wl_state *)data;
    state->width = width;
    state->height = height;
    state->configured = true;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void layer_surface_closed(void *data,
        struct zwlr_layer_surface_v1 *surface) {
    (void)surface;
    auto *state = (struct wl_state *)data;
    state->running = false;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure,
    .closed = layer_surface_closed,
};

// pointer: any event = quit
static void pointer_enter(void *data, struct wl_pointer *p, uint32_t serial,
        struct wl_surface *s, wl_fixed_t x, wl_fixed_t y) {
    (void)data; (void)p; (void)serial; (void)s; (void)x; (void)y;
}

static void pointer_leave(void *data, struct wl_pointer *p, uint32_t serial,
        struct wl_surface *s) {
    (void)data; (void)p; (void)serial; (void)s;
}

static void pointer_motion(void *data, struct wl_pointer *p, uint32_t time,
        wl_fixed_t x, wl_fixed_t y) {
    (void)p; (void)time; (void)x; (void)y;
    auto *state = (struct wl_state *)data;
    state->running = false;
}

static void pointer_button(void *data, struct wl_pointer *p, uint32_t serial,
        uint32_t time, uint32_t button, uint32_t s) {
    (void)p; (void)serial; (void)time; (void)button; (void)s;
    auto *state = (struct wl_state *)data;
    state->running = false;
}

static void pointer_axis(void *data, struct wl_pointer *p, uint32_t time,
        uint32_t axis, wl_fixed_t value) {
    (void)p; (void)time; (void)axis; (void)value;
    auto *state = (struct wl_state *)data;
    state->running = false;
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis,
};

// keyboard: any event = quit
static void keyboard_keymap(void *data, struct wl_keyboard *kb, uint32_t fmt,
        int32_t fd, uint32_t size) {
    (void)data; (void)kb; (void)fmt; (void)fd; (void)size;
}

static void keyboard_enter(void *data, struct wl_keyboard *kb, uint32_t serial,
        struct wl_surface *s, struct wl_array *keys) {
    (void)data; (void)kb; (void)serial; (void)s; (void)keys;
}

static void keyboard_leave(void *data, struct wl_keyboard *kb, uint32_t serial,
        struct wl_surface *s) {
    (void)data; (void)kb; (void)serial; (void)s;
}

static void keyboard_key(void *data, struct wl_keyboard *kb, uint32_t serial,
        uint32_t time, uint32_t key, uint32_t s) {
    (void)kb; (void)serial; (void)time; (void)key; (void)s;
    auto *state = (struct wl_state *)data;
    state->running = false;
}

static void keyboard_modifiers(void *data, struct wl_keyboard *kb,
        uint32_t serial, uint32_t dep, uint32_t lat, uint32_t lock,
        uint32_t group) {
    (void)data; (void)kb; (void)serial; (void)dep; (void)lat; (void)lock;
    (void)group;
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
};

// seat: grab pointer and keyboard when available
static void seat_capabilities(void *data, struct wl_seat *seat,
        uint32_t caps) {
    auto *state = (struct wl_state *)data;
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !state->pointer) {
        state->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(state->pointer, &pointer_listener, state);
    }
    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !state->keyboard) {
        state->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(state->keyboard, &keyboard_listener, state);
    }
}

static void seat_name(void *data, struct wl_seat *seat, const char *name) {
    (void)data; (void)seat; (void)name;
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name,
};

// output: get scale
static void output_geometry(void *data, struct wl_output *o, int32_t x,
        int32_t y, int32_t pw, int32_t ph, int32_t subpix, const char *make,
        const char *model, int32_t transform) {
    (void)data; (void)o; (void)x; (void)y; (void)pw; (void)ph;
    (void)subpix; (void)make; (void)model; (void)transform;
}

static void output_mode(void *data, struct wl_output *o, uint32_t flags,
        int32_t w, int32_t h, int32_t refresh) {
    (void)data; (void)o; (void)flags; (void)w; (void)h; (void)refresh;
}

static void output_scale(void *data, struct wl_output *o, int32_t factor) {
    (void)o;
    auto *state = (struct wl_state *)data;
    state->scale = factor;
}

static void output_done(void *data, struct wl_output *o) {
    (void)data; (void)o;
}

static const struct wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode = output_mode,
    .done = output_done,
    .scale = output_scale,
};

static void registry_global(void *data, struct wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version) {
    (void)version;
    auto *state = (struct wl_state *)data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = (struct wl_compositor *)wl_registry_bind(
            registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        state->layer_shell = (struct zwlr_layer_shell_v1 *)wl_registry_bind(
            registry, name, &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, wl_output_interface.name) == 0 && !state->output) {
        state->output = (struct wl_output *)wl_registry_bind(
            registry, name, &wl_output_interface, 2);
        wl_output_add_listener(state->output, &output_listener, state);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        state->seat = (struct wl_seat *)wl_registry_bind(
            registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(state->seat, &seat_listener, state);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry,
        uint32_t name) {
    (void)data; (void)registry; (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

bool wayland_init(struct wl_state *state) {
    memset(state, 0, sizeof(*state));
    state->running = true;

    state->display = wl_display_connect(nullptr);
    if (!state->display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return false;
    }

    state->scale = 1;

    state->registry = wl_display_get_registry(state->display);
    wl_registry_add_listener(state->registry, &registry_listener, state);
    wl_display_roundtrip(state->display);
    // second roundtrip to receive output events (scale)
    wl_display_roundtrip(state->display);

    if (!state->compositor) {
        fprintf(stderr, "No wl_compositor found\n");
        return false;
    }
    if (!state->layer_shell) {
        fprintf(stderr, "No zwlr_layer_shell_v1 found\n");
        return false;
    }

    state->surface = wl_compositor_create_surface(state->compositor);

    state->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        state->layer_shell, state->surface, nullptr,
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "wiping");

    zwlr_layer_surface_v1_set_anchor(state->layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(state->layer_surface, -1);
    zwlr_layer_surface_v1_set_keyboard_interactivity(state->layer_surface,
        ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
    zwlr_layer_surface_v1_add_listener(state->layer_surface,
        &layer_surface_listener, state);

    wl_surface_commit(state->surface);
    while (!state->configured && state->running) {
        wl_display_dispatch(state->display);
    }

    if (!state->configured) return false;

    wl_surface_set_buffer_scale(state->surface, state->scale);
    state->width *= state->scale;
    state->height *= state->scale;

    state->egl_window = wl_egl_window_create(state->surface,
        state->width, state->height);
    if (!state->egl_window) {
        fprintf(stderr, "Failed to create EGL window\n");
        return false;
    }

    return true;
}

void wayland_cleanup(struct wl_state *state) {
    if (state->keyboard)
        wl_keyboard_destroy(state->keyboard);
    if (state->pointer)
        wl_pointer_destroy(state->pointer);
    if (state->seat)
        wl_seat_destroy(state->seat);
    if (state->output)
        wl_output_destroy(state->output);
    if (state->egl_window)
        wl_egl_window_destroy(state->egl_window);
    if (state->layer_surface)
        zwlr_layer_surface_v1_destroy(state->layer_surface);
    if (state->surface)
        wl_surface_destroy(state->surface);
    if (state->layer_shell)
        zwlr_layer_shell_v1_destroy(state->layer_shell);
    if (state->compositor)
        wl_compositor_destroy(state->compositor);
    if (state->registry)
        wl_registry_destroy(state->registry);
    if (state->display)
        wl_display_disconnect(state->display);
}
