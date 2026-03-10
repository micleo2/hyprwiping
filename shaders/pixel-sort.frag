// ASDF-style pixel sorting via odd-even transposition sort.
//
// Adapted from Kim Asendorf's ASDFPixelSort algorithm.
// The original screenshot defines span boundaries — dark pixels are barriers.
// Within spans, pixels are incrementally sorted each frame using parallel
// odd-even compare-swap. Sorts both columns and rows (alternating frames).

precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_texture;
uniform sampler2D u_screenshot;
uniform float u_time;
uniform int u_frame;
uniform vec2 u_resolution;

float luma(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

// Sort key: luminance, with slight red priority to echo Processing's
// integer color sort aesthetic.
float sortKey(vec3 c) {
    return dot(c, vec3(0.4, 0.35, 0.25));
}

void main() {
    // Frame 0: seed from screenshot
    if (u_frame == 0) {
        gl_FragColor = texture2D(u_screenshot, v_uv);
        return;
    }

    vec2 px = 1.0 / u_resolution;
    float x = floor(v_uv.x * u_resolution.x);
    float y = floor(v_uv.y * u_resolution.y);
    float frame = float(u_frame);

    vec4 self = texture2D(u_texture, v_uv);

    // Barrier detection from original screenshot — ASDF "black" mode.
    // Only very dark pixels act as barriers. This is a low threshold
    // matching ASDF's blackValue which lets most of the image sort.
    float origLum = luma(texture2D(u_screenshot, v_uv).rgb);
    float threshold = 0.08;

    if (origLum < threshold) {
        gl_FragColor = self;
        return;
    }

    // Column sorting only — vertical streaks
    float phase = mod(frame, 2.0);
    vec2 step = vec2(0.0, px.y);
    float coord = y;

    // Odd-even pair assignment
    bool isFirst = mod(coord + phase, 2.0) < 0.5;
    vec2 neighborUV = v_uv + (isFirst ? step : -step);

    // Boundary check
    if (neighborUV.x < 0.0 || neighborUV.x > 1.0 ||
        neighborUV.y < 0.0 || neighborUV.y > 1.0) {
        gl_FragColor = self;
        return;
    }

    // Neighbor must also be in a sortable span
    float origNeighborLum = luma(texture2D(u_screenshot, neighborUV).rgb);
    if (origNeighborLum < threshold) {
        gl_FragColor = self;
        return;
    }

    // Compare-swap by sort key
    vec4 neighbor = texture2D(u_texture, neighborUV);
    float selfKey = sortKey(self.rgb);
    float neighborKey = sortKey(neighbor.rgb);

    // Ascending: first-of-pair keeps smaller key, second keeps larger
    bool needSwap;
    if (isFirst) {
        needSwap = selfKey > neighborKey;
    } else {
        needSwap = selfKey < neighborKey;
    }

    gl_FragColor = needSwap ? neighbor : self;
}
