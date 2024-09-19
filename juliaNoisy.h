const char* fragmentShaderSource3 = R"(
#version 330 core
out vec4 FragColor;
uniform vec2 u_resolution;
uniform vec2 u_offset;
uniform float u_zoom;
uniform vec2 u_c;
const int MAX_ITER = 200;
const int NUM_SAMPLES = 4;
uniform float c_parameter_r;
uniform float c_parameter_g;
uniform float c_parameter_b;
uniform float amplitude;  // New uniform for amplitude input

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

// Function to generate pseudo-random noise based on coordinates
float randomNoise(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453);
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
        vec2 noisyCoord;
        if(abs(amplitude) > 0.9){
            // Generate noise based on fragment coordinates
            float noiseX = randomNoise(sampleCoord * 0.1) * amplitude; // Perturb x with noise and amplitude
            float noiseY = randomNoise(sampleCoord * 0.1 + 10.0) * amplitude; // Perturb y with noise and amplitude

            // Add the noise to the coordinates before passing them to the fractal function
            noisyCoord = sampleCoord + vec2(noiseX, noiseY)/50;
        }else{
            noisyCoord = sampleCoord;
        }


        int iterations = julia(noisyCoord);
        float t = float(iterations) / MAX_ITER;

        color += vec3(t * 9.0 * c_parameter_r / 2, t * 9.0 * c_parameter_g / 2, t * 9.0 * c_parameter_b / 2);
        if (iterations == MAX_ITER) color += vec3(0.0);
    }

    // Average the color
    color /= float(NUM_SAMPLES);

    // Final color output
    FragColor = vec4(color, 1.0);
}
)";