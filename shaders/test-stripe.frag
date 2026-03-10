precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_screenshot;
uniform float u_time;

void main() {
    vec4 color = texture2D(u_screenshot, v_uv);

    // stripe is 1/3 screen width, bouncing left to right
    float stripe_w = 1.0 / 3.0;
    float range = 1.0 - stripe_w;
    float pos = mod(u_time * 0.3, range * 2.0);
    if (pos > range) pos = range * 2.0 - pos;

    if (v_uv.x > pos && v_uv.x < pos + stripe_w) {
        color = mix(color, vec4(1.0, 0.0, 0.0, 1.0), 0.5);
    }
    gl_FragColor = color;
}
