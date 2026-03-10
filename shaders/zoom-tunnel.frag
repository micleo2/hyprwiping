precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_texture;
uniform float u_time;
uniform int u_frame;
uniform vec2 u_resolution;

void main() {
    // zoom toward center
    vec2 center = vec2(0.5);
    vec2 uv = mix(v_uv, center, 0.003);

    vec4 color = texture2D(u_texture, uv);

    // subtle hue rotation over time
    float angle = u_time * 0.01;
    float s = sin(angle);
    float c = cos(angle);
    vec3 rgb = color.rgb;
    color.rgb = vec3(
        rgb.r * c + rgb.g * -s,
        rgb.r * s + rgb.g * c,
        rgb.b
    );

    color *= 0.999;
    gl_FragColor = color;
}
