precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_texture;
uniform float u_time;
uniform int u_frame;
uniform vec2 u_resolution;

void main() {
    vec2 pixel = 1.0 / u_resolution;

    // slow drift toward upper-right
    vec2 uv = v_uv + vec2(0.0008, 0.0004);

    // 3x3 box blur
    vec4 color = vec4(0.0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            color += texture2D(u_texture, uv + vec2(float(x), float(y)) * pixel);
        }
    }
    color /= 9.0;

    // slight decay to fade toward black over time
    color *= 0.998;

    gl_FragColor = color;
}
