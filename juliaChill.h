const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec2 u_resolution;
uniform vec2 u_offset;
uniform float u_zoom;
uniform vec2 u_c;
const int MAX_ITER = 2000;
const int NUM_SAMPLES = 4;
uniform float c_parameter_r;
uniform float c_parameter_g;
uniform float c_parameter_b;
uniform float amplitube;

int julia(vec2 z) {
    int iterations = 0;
    for (int i = 0; i < MAX_ITER; i++) {
        if (length(z) > 2.0) break;
        float xTemp = z.x * z.x - z.y * z.y + u_c.x;
        z.y = 2.0 * z.x * z.y + u_c.y;
        z.x = xTemp;
        iterations++;
    }
    return iterations;
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
    for (int i = 0; i < NUM_SAMPLES; i++) {
        vec2 sampleCoord = coord + (offsets[i] / u_resolution) * u_zoom;
        int iterations = julia(sampleCoord);
        float t = float(iterations) / MAX_ITER;

        color += vec3(t * 9.0 * c_parameter_r / 2, t * 9.0 * c_parameter_g / 2, t * 9.0 * c_parameter_b / 2);
        if (iterations == MAX_ITER) color += vec3(0.0);
    }

    // Average the color
    color /= float(NUM_SAMPLES);

    FragColor = vec4(color, 1.0);
}
)";


// const char* fragmentShaderSourcee = R"(
// #version 330 core
// out vec4 FragColor;
// uniform vec2 u_resolution;
// uniform vec2 u_offset;
// uniform float u_zoom;
// uniform vec2 u_c;
// const int MAX_ITER = 2000;
// const int NUM_SAMPLES = 4;
// uniform float c_parameter_r;
// uniform float c_parameter_g;
// uniform float c_parameter_b;
// uniform float amplitude;

// int julia1(vec2 z, vec2 c) {
//     int iterations = 0;
//     for (int i = 0; i < MAX_ITER; i++) {
//         if (length(z) > 2.0) break;
//         float xTemp = z.x * z.x - z.y * z.y + c.x;
//         z.y = 2.0 * z.x * z.y + c.y;
//         z.x = xTemp;
//         iterations++;
//     }
//     return iterations;
// }

// // Second Julia fractal function (with a different parameter c)
// int julia2(vec2 z, vec2 c) {
//     int iterations = 0;
//     for (int i = 0; i < MAX_ITER; i++) {
//         if (length(z) > 2.0) break;
//         float xTemp = z.x * z.x - z.y * z.y + c.x * 1.5; // Example variation for second fractal
//         z.y = 2.0 * z.x * z.y + c.y * 1.5;               // Scaling the 'c' parameter for variety
//         z.x = xTemp;
//         iterations++;
//     }
//     return iterations;
// }

// void main() {
//     // Calculate aspect ratio
//     float aspectRatio = u_resolution.x / u_resolution.y;

//     // Adjust the coordinate to maintain the aspect ratio
//     vec2 coord = (gl_FragCoord.xy / u_resolution - 0.5) * vec2(aspectRatio, 1.0) * u_zoom + u_offset;

//     // Supersampling offsets
//     vec2 offsets[NUM_SAMPLES] = vec2[](
//         vec2(-0.25, -0.25), vec2(0.25, -0.25),
//         vec2(-0.25, 0.25), vec2(0.25, 0.25)
//     );

//     // Accumulate color over multiple samples
//     vec3 color = vec3(0.0);
//     for (int i = 0; i < NUM_SAMPLES; i++) {
//         vec2 sampleCoord = coord + (offsets[i] / u_resolution) * u_zoom;
        
//         // Compute iterations for the two fractals
//         int iterations1 = julia1(sampleCoord, u_c);
//         int iterations2 = julia2(sampleCoord, u_c * 1.2);  // Slightly different constant
        
//         // Blend the two fractal iterations
//         float blendFactor = amplitude*2;  // You can adjust this or make it dynamic
//         float t1 = float(iterations1) / MAX_ITER;
//         float t2 = float(iterations2) / MAX_ITER;
//         float blended = mix(t1, t2, blendFactor);
        
//         color += vec3(blended * 9.0 * c_parameter_r / 2, blended * 9.0 * c_parameter_g / 2, blended * 9.0 * c_parameter_b / 2);
//         if (iterations1 == MAX_ITER || iterations2 == MAX_ITER) color += vec3(0.0);
//     }

//     // Average the color
//     color /= float(NUM_SAMPLES);

//     FragColor = vec4(color, 1.0);
// }
// )";
