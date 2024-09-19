const char* fragmentShaderSource2 = R"(
#version 330 core
out vec4 FragColor;
uniform vec2 u_resolution;
uniform vec2 u_offset;
uniform float u_zoom;
uniform vec2 u_c;
const int MAX_ITER = 200;
const int NUM_SAMPLES = 4; // Ensure this matches your supersampling setup
uniform float c_parameter_r;
uniform float c_parameter_g;
uniform float c_parameter_b;

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

    // Supersampling offsets for 8x MSAA
    // vec2 offsets[NUM_SAMPLES] = vec2[](
    //     vec2(-0.125, -0.375), vec2(0.125, -0.375),
    //     vec2(-0.375, -0.125), vec2(0.375, -0.125),
    //     vec2(-0.125, 0.125), vec2(0.125, 0.125),
    //     vec2(-0.375, 0.375), vec2(0.375, 0.375)
    // );
    vec2 offsets[NUM_SAMPLES] = vec2[](
        vec2(-0.25, -0.25), vec2(0.25, -0.25),
        vec2(-0.25, 0.25), vec2(0.25, 0.25)
    );
    // Accumulate color over multiple samples
    vec3 color = vec3(0.0);
    for (int i = 0; i < NUM_SAMPLES; i++) {
        vec2 sampleCoord = coord + (offsets[i] / u_resolution) * u_zoom;
        int iterations = julia(sampleCoord);
        float t = float(iterations) / MAX_ITER; // Normalize iteration count

        // Enhanced color transition using multiple sine waves for vibrant effect
        vec3 sampleColor = vec3( // + 0.5 se ola itan kanonika alla to evgala!!!!!!!!!!!!!!!!!!!!S 
            0.5 + 0.5 * sin(6.0 * t + c_parameter_r * 6.28318),
            0.5 + 0.5 * sin(6.0 * t + c_parameter_g * 6.28318 + 2.094),
            0.5 + 0.5 * sin(6.0 * t + c_parameter_b * 6.28318 + 4.188)
        );
        // sampleColor = vec3(1.0) - sampleColor; 

        // Enhance brightness and contrast using a power function
        sampleColor = pow(sampleColor, vec3(0.8)); // Control this exponent for different effects

        // Accumulate the sample color
        color += sampleColor;

                // Invert color if the point is part of the fractal (iterations == MAX_ITER)
        if (iterations == MAX_ITER) {
            sampleColor = vec3(1.0) - sampleColor; // Invert the color
        }
    }

    // Average the color and apply final intensity adjustment
    color = color / float(NUM_SAMPLES) * 0.8; // Adjust final intensity for better visual balance

    FragColor = vec4(color, 1.0);
}
)";