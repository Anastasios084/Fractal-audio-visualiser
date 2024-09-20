const char* fragmentShaderSource5 = R"(
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
uniform float amplitude;
uniform float u_time;
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
            o = (0.5 + 0.5 * sin(1 + 6.2831 * o * continuous_time));

            // Distance to cell
            float d = length(g - f + o);
            
             // Cell color with RGB control
            vec3 col = 0.5 + 0.5 * sin(hash1(dot(n + g, vec2(7.0, 113.0))) * 2.5 + 3.5 + vec3(2.0, 3.0, 0.0));

            // Adjust colors using the uniform parameters
            col.r *= c_parameter_r/5; // Adjust red
            col.g *= c_parameter_g/5; // Adjust green
            col.b *= c_parameter_b/5; // Adjust blue

            col = col * col; // In linear space
            float darkenFactor = smoothstep(1.0, 0.0, d); // Gradual darkening towards the center
            col *= (1.0 - darkenFactor/2); // Make the center darker
            // Smooth min for colors and distances
            float h = smoothstep(-1.0, 1.0, (m.x - d) / w);
            m.x = mix(m.x, d, h) - h * (1.0 - h) * w / (1.0 + 3.0 * w); // Distance
            m.yzw = mix(m.yzw, col, h) - h * (1.0 - h) * w / (1.0 + 3.0 * w); // Color
        }
    }

    return m;
}

void main() {
    // Calculate aspect ratio
    float aspectRatio = u_resolution.x / u_resolution.y;

    // Adjust the coordinate to maintain the aspect ratio
    vec2 p = (gl_FragCoord.xy / u_resolution - 0.5) * vec2(aspectRatio, 1.0) * u_zoom + u_offset;

    // Normalize the fragment coordinates
    // vec2 p = gl_FragCoord.xy / u_resolution;

    float c = 0.5 * u_resolution.x / u_resolution.y;
    vec4 v = voronoi(50.0 * p, 0.1);

    // Gamma correction
    vec3 col = sqrt(v.yzw);
    
    // Remove horizontal splits
    col *= smoothstep(0.003, 0.005, abs(p.x - c));

    FragColor = vec4(col, 1.0);
}


)";
