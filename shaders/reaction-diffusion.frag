precision mediump float;
varying vec2 v_uv;
uniform sampler2D u_texture;
uniform sampler2D u_screenshot;
uniform float u_time;
uniform int u_frame;
uniform vec2 u_resolution;

void main() {
    vec2 pixel = 1.0 / u_resolution;

    // frame 0: seed from screenshot
    if (u_frame == 0) {
        vec4 img = texture2D(u_screenshot, v_uv);
        float lum = dot(img.rgb, vec3(0.299, 0.587, 0.114));
        // U = 1.0 everywhere, V seeded at edges/detail
        float U = 1.0;
        // seed V where there's detail: use local contrast
        float lum_r = dot(texture2D(u_screenshot, v_uv + vec2(pixel.x, 0.0)).rgb, vec3(0.299, 0.587, 0.114));
        float lum_u = dot(texture2D(u_screenshot, v_uv + vec2(0.0, pixel.y)).rgb, vec3(0.299, 0.587, 0.114));
        float edge = abs(lum - lum_r) + abs(lum - lum_u);
        float V = smoothstep(0.01, 0.15, edge);
        gl_FragColor = vec4(U, V, 0.0, 1.0);
        return;
    }

    // Gray-Scott parameters
    // f=0.037 k=0.06 gives coral/maze-like growth
    float f = 0.037;
    float k = 0.06;
    float Du = 0.21;
    float Dv = 0.105;
    float dt = 0.4;

    vec4 c = texture2D(u_texture, v_uv);
    float U = c.r;
    float V = c.g;

    // 3x3 Laplacian
    float lapU = -U;
    float lapV = -V;
    // edge weights: 0.2, corner weights: 0.05
    lapU += texture2D(u_texture, v_uv + vec2(-pixel.x, 0.0)).r * 0.2;
    lapU += texture2D(u_texture, v_uv + vec2( pixel.x, 0.0)).r * 0.2;
    lapU += texture2D(u_texture, v_uv + vec2(0.0, -pixel.y)).r * 0.2;
    lapU += texture2D(u_texture, v_uv + vec2(0.0,  pixel.y)).r * 0.2;
    lapU += texture2D(u_texture, v_uv + vec2(-pixel.x, -pixel.y)).r * 0.05;
    lapU += texture2D(u_texture, v_uv + vec2( pixel.x, -pixel.y)).r * 0.05;
    lapU += texture2D(u_texture, v_uv + vec2(-pixel.x,  pixel.y)).r * 0.05;
    lapU += texture2D(u_texture, v_uv + vec2( pixel.x,  pixel.y)).r * 0.05;

    lapV += texture2D(u_texture, v_uv + vec2(-pixel.x, 0.0)).g * 0.2;
    lapV += texture2D(u_texture, v_uv + vec2( pixel.x, 0.0)).g * 0.2;
    lapV += texture2D(u_texture, v_uv + vec2(0.0, -pixel.y)).g * 0.2;
    lapV += texture2D(u_texture, v_uv + vec2(0.0,  pixel.y)).g * 0.2;
    lapV += texture2D(u_texture, v_uv + vec2(-pixel.x, -pixel.y)).g * 0.05;
    lapV += texture2D(u_texture, v_uv + vec2( pixel.x, -pixel.y)).g * 0.05;
    lapV += texture2D(u_texture, v_uv + vec2(-pixel.x,  pixel.y)).g * 0.05;
    lapV += texture2D(u_texture, v_uv + vec2( pixel.x,  pixel.y)).g * 0.05;

    float uvv = U * V * V;
    float newU = U + (Du * lapU - uvv + f * (1.0 - U)) * dt;
    float newV = V + (Dv * lapV + uvv - (f + k) * V) * dt;

    gl_FragColor = vec4(clamp(newU, 0.0, 1.0), clamp(newV, 0.0, 1.0), 0.0, 1.0);
}
