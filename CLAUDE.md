# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

A GPU-accelerated screensaver for Hyprland (Wayland). Takes a screenshot of the desktop, applies real-time fragment shader effects to it as a fullscreen overlay, and dismisses on any input.

## Build

CMake + Ninja + clang. Build artifacts live under `nasignore/`.

```sh
# Debug (has compile_commands.json symlinked to project root)
cmake -S . -B nasignore/debug -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug
ninja -C nasignore/debug

# Release
cmake -S . -B nasignore/release -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release
ninja -C nasignore/release
```

## Run

```sh
# Quick start (takes screenshot via grim, launches release binary)
scripts/wiping-start shaders/rain.frag

# Manual
grim /tmp/ss.png
nasignore/release/wiping --shader shaders/rain.frag --screenshot /tmp/ss.png

# With separate display shader (for simulation-type effects)
nasignore/release/wiping --shader shaders/reaction-diffusion.frag \
  --display shaders/reaction-diffusion-display.frag --screenshot /tmp/ss.png

# Stop
scripts/wiping-stop
```

## Architecture

### Rendering Pipeline

Two-pass ping-pong FBO rendering per frame:

1. **Feedback pass**: Reads FBO-A + screenshot texture, writes to FBO-B via `--shader`
2. **Display pass**: Reads FBO-B + screenshot texture, writes to screen via `--display` (or passthrough if omitted)
3. Swap: FBO-B becomes FBO-A next frame

### Shader Uniforms

All shaders receive these uniforms (unused ones are silently ignored):

| Uniform | Type | Description |
|---------|------|-------------|
| `u_texture` | `sampler2D` | Previous frame's FBO output (feedback) |
| `u_screenshot` | `sampler2D` | Immutable original screenshot |
| `u_time` | `float` | Seconds since start |
| `u_frame` | `int` | Frame counter (0-based) |
| `u_resolution` | `vec2` | Viewport size in pixels |

Shaders are GLSL ES 1.00 (`precision mediump float;`, `varying`, `gl_FragColor`). No `textureLod` — use manual sampling for blur effects.

### Source Layout

- `src/main.cpp` — Entry point, CLI parsing, Wayland frame callback chain
- `src/wayland.cpp` — Wayland setup via wlr-layer-shell (OVERLAY layer, exclusive keyboard, dismisses on any input)
- `src/render.cpp` — EGL init, FBO ping-pong, shader compilation, two-pass draw loop
- `src/wayland.h` / `src/render.h` — State structs and declarations
- `shaders/` — Fragment shaders (the creative part)
- `protocol/` — Vendored wlr-layer-shell-unstable-v1.xml

### Key Gotchas

- **C++ with C protocols**: Generated Wayland headers use `namespace` as a parameter name. The workaround is `#define namespace namespace_` around the include (see `wayland.h`).
- **Time precision**: `start_time` is `double`. Only cast the delta to `float` for the uniform — computing time as `float` from epoch loses all sub-second precision.
- **Frame callback ordering**: `wl_surface_frame()` must be called BEFORE `eglSwapBuffers` (inside `render_frame`), not after.
- **HiDPI**: Logical size from layer-shell configure is multiplied by `wl_output` scale factor. `wl_surface_set_buffer_scale` must be called.
- **Image orientation**: `stbi_set_flip_vertically_on_load(1)` is required — OpenGL UV origin is bottom-left.
