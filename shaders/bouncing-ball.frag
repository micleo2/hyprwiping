precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_screenshot;
uniform float u_time;
uniform vec2 u_resolution;

void main() {
    vec3 color = texture2D(u_screenshot, v_uv).rgb;

    // ball properties
    float radius = 30.0;
    vec2 speed = vec2(300.0, 200.0);

    // bounce: ping-pong across screen in pixel coords
    vec2 range = u_resolution - vec2(radius * 2.0);
    vec2 pos = mod(speed * u_time, range * 2.0);
    // reflect: if past range, fold back
    if (pos.x > range.x) pos.x = range.x * 2.0 - pos.x;
    if (pos.y > range.y) pos.y = range.y * 2.0 - pos.y;
    pos += vec2(radius);

    // draw ball
    vec2 pixel = v_uv * u_resolution;
    float dist = length(pixel - pos);
    if (dist < radius) {
        color = vec3(1.0, 0.2, 0.2);
    }

    gl_FragColor = vec4(color, 1.0);
}
