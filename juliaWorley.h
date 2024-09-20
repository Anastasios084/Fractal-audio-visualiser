const char* fragmentShaderSource6 = R"(
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
uniform float amplitude;
uniform float continuous_time;

float hash1( float n ) { return fract(sin(n)*43758.5453); }
vec2  hash2( vec2  p ) { 
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3))); 
    return fract(sin(p)*43758.5453); 
}

// The parameter w controls the smoothness
vec4 voronoi(in vec2 x, float w) {
    vec2 n = floor(x);
    vec2 f = fract(x);

    vec4 m = vec4(8.0, 0.0, 0.0, 0.0);
    for (int j = -2; j <= 2; j++) {
        for (int i = -2; i <= 2; i++) {
            vec2 g = vec2(float(i), float(j));
            vec2 o = hash2(n + g);
            
            // Animate
            o = (0.5 + (abs(amplitude)+0.5) * sin(1 + 6.2831 * o * continuous_time));

            // Distance to cell
            float d = length(g - f + o);
            
             // Cell color with RGB control
            vec3 col = 0.5 + 0.5 * sin(hash1(dot(n + g, vec2(7.0, 113.0))) * 2.5 + 3.5 + vec3(2.0, 3.0, 0.0));

            // Adjust colors using the uniform parameters
            col.r *= c_parameter_r; // Adjust red
            col.g *= c_parameter_g; // Adjust green
            col.b *= c_parameter_b; // Adjust blue

            col = col * col; // In linear space
            float darkenFactor = smoothstep(0.0, 1.0, d); // Gradual darkening towards the center
            col *= (1.0 - darkenFactor); // Make the center darker
            // Smooth min for colors and distances
            float h = smoothstep(-1.0, 1.0, (m.x - d) / w);
            m.x = mix(m.x, d, h) - h * (1.0 - h) * w / (1.0 + 3.0 * w); // Distance
            m.yzw = mix(m.yzw, col, h) - h * (1.0 - h) * w / (1.0 + 3.0 * w); // Color
        }
    }

    return m;
}



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
    vec3 color = vec3(0.0);

    // Adjust the coordinate to maintain the aspect ratio
    vec2 coord = (gl_FragCoord.xy / u_resolution - 0.5) * vec2(aspectRatio, 1.0) * u_zoom + u_offset;

    // Adjust the coordinate to maintain the aspect ratio
    vec2 p = (gl_FragCoord.xy / u_resolution - 0.5) * vec2(aspectRatio, 1.0) * u_zoom + u_offset;

    // Normalize the fragment coordinates
    // vec2 p = gl_FragCoord.xy / u_resolution;

    float c = 0.5 * u_resolution.x / u_resolution.y;
    vec4 v = voronoi(100.0 * p, 0.3);

    // Gamma correction
    vec3 col = sqrt(v.yzw);
    
    // Remove horizontal splits
    col *= smoothstep(0.003, 0.005, abs(p.x - c))/2;
    col = pow(col, vec3(2.0)); // Apply gamma correction to intensify color differences
    // Supersampling offsets
    vec2 offsets[NUM_SAMPLES] = vec2[](
        vec2(-0.25, -0.25), vec2(0.25, -0.25),
        vec2(-0.25, 0.25), vec2(0.25, 0.25)
    );

    // Accumulate color over multiple samples
    float amp = amplitude;
    float average_color_intensity = (c_parameter_r + c_parameter_g + c_parameter_b)/10;
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

        // Special color for maximum iteration
        if (iterations1 == MAX_ITER || iterations2 == MAX_ITER) {
            color += col;  // Gold for escape
        }else{
            // Accumulate the blended color
            color += blendedColor;
            color += color1 * 0.5;
        }

    }

    // Average the color over the samples
    color /= float(NUM_SAMPLES);

    FragColor = vec4(color, 1.0);
}
)";
