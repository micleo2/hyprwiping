precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_texture;
uniform float u_time;
uniform int u_frame;
uniform vec2 u_resolution;

// simple hash
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec4 color = texture2D(u_texture, v_uv);

    // randomly darken pixels
    float r = rand(v_uv + vec2(u_time));
    if (r < 0.02) {
        color *= 0.5;
    }

    // channel drift: shift red slightly
    vec2 pixel = 1.0 / u_resolution;
    float red = texture2D(u_texture, v_uv + pixel * vec2(1.0, 0.0)).r;
    color.r = mix(color.r, red, 0.1);

    color *= 0.9995;
    gl_FragColor = color;
}
