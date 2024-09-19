const char* fragmentShaderSource4 = R"(
#version 330 core
out vec4 FragColor;
uniform vec2 u_resolution;
uniform vec2 u_offset;
uniform float u_zoom;
uniform vec2 u_c;
const int MAX_ITER = 500;
const int NUM_SAMPLES = 4;
uniform float c_parameter_r;
uniform float c_parameter_g;
uniform float c_parameter_b;
uniform float amplitude;

// Julia fractal 1
int julia1(vec2 z, vec2 c) {
    int iterations = 0;
    for (int i = 0; i < MAX_ITER; i++) {
        if (length(z) > 2.0) break;
        float xTemp = z.x * z.x - z.y * z.y + c.x;
        z.y = 2.0 * z.x * z.y + c.y;
        z.x = xTemp;
        iterations++;
    }
    return iterations;
}

// Julia fractal 2 (with different parameter c)
int julia2(vec2 z, vec2 c) {
    int iterations = 0;
    for (int i = 0; i < MAX_ITER; i++) {
        if (length(z) > 2.0) break;
        float xTemp = z.x * z.x - z.y * z.y + c.x * 1.5;
        z.y = 2.0 * z.x * z.y + c.y * 1.5;
        z.x = xTemp;
        iterations++;
    }
    return iterations;
}

// Sigmoid function for smooth blending
float sigmoid(float x) {
    return 1.0 / (1.0 + exp(-x));
}

void main() {
    // Calculate aspect ratio
    float aspectRatio = u_resolution.x / u_resolution.y;

    // Adjust the coordinate to maintain the aspect ratio
    vec2 coord = (gl_FragCoord.xy / u_resolution - 0.5) * vec2(aspectRatio, 1.0) * u_zoom + u_offset;

    // Supersampling offsets
    vec2 offsets[NUM_SAMPLES] = vec2[](
        vec2(-0.25, -0.25), vec2(0.25, -0.25),
        vec2(-0.25, 0.25), vec2(0.25, 0.25)
    );

    // Accumulate color over multiple samples
    vec3 color = vec3(0.0);
    float amp = amplitude;
    float average_color_intensity = (c_parameter_r + c_parameter_g + c_parameter_b)/3;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        vec2 sampleCoord = coord;

        // Compute iterations for the two fractals
        int iterations1 = julia1(sampleCoord, u_c);
        int iterations2 = julia2(sampleCoord, u_c * 0.9);

        // Smooth blending factor for transitioning between fractals
        float blendFactor = (amp-0.2) * 10.0;

        // Normalize iteration counts to [0, 1]
        float t1 = float(iterations1) / MAX_ITER;
        float t2 = float(iterations2) / MAX_ITER;

        // Compute colors for ea 9.0 * ch fractal
        vec3 color1 = vec3(t1 * c_parameter_r, t1 * c_parameter_g, t1 * c_parameter_b) * 0.5;
        vec3 color2 = vec3(t2*2 * average_color_intensity*2 , t2 * average_color_intensity*2, t2/2 * average_color_intensity*2);  // Gold color for second fractal

        // Blend the colors using the blend factor
        vec3 blendedColor = mix(color2, color1, blendFactor);

        // Accumulate the blended color
        color += blendedColor;
        color += color1 * 0.5;
        // Special color for maximum iteration
        if (iterations1 == MAX_ITER || iterations2 == MAX_ITER) {
            color += vec3(0.0, 0.0, 0.0);  // Gold for escape
        }
    }

    // Average the color over the samples
    color /= float(NUM_SAMPLES);

    FragColor = vec4(color, 1.0);
}
)";
