precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_texture;
uniform sampler2D u_screenshot;
uniform float u_time;
uniform vec2 u_resolution;

void main() {
    vec4 sim = texture2D(u_texture, v_uv);
    vec3 img = texture2D(u_screenshot, v_uv).rgb;
    float V = sim.g;

    // strong darkening where the pattern grows
    vec3 color = img * (1.0 - V);

    // replace darkened areas with a deep color
    vec3 pattern = vec3(0.02, 0.06, 0.12);
    color = mix(img, pattern, V);

    gl_FragColor = vec4(color, 1.0);
}
