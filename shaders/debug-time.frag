precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_screenshot;
uniform float u_time;
uniform vec2 u_resolution;

void main() {
    vec3 color = texture2D(u_screenshot, v_uv).rgb;

    // pulse the entire screen red based on time
    float pulse = sin(u_time * 3.0) * 0.5 + 0.5;
    color = mix(color, vec3(1.0, 0.0, 0.0), pulse * 0.5);

    gl_FragColor = vec4(color, 1.0);
}
