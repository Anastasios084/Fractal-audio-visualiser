#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>


#include <fstream>
#include <sstream>
#include <string>

#define RESOLUTION_W 2560
#define RESOLUTION_H 1080

const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec2 in_position;

void main() {
    gl_Position = vec4(in_position, 0.0, 1.0);
}
)";


const char* fragmentShaderSource = R"(
#version 330 core
layout(location = 0) out vec4 fragColor;

uniform vec2 u_resolution;
uniform float u_time;

const int MAX_STEPS = 300;
const float MAX_DIST = 50;
const float EPSILON = 0.0001;
const float PI = acos(-1.0);


mat2 rot(float a) {
    float ca = cos(a);
    float sa = sin(a);
    return mat2(ca, -sa, sa, ca);
}


float getSphere(vec3 p, float r) {
    return length(p) - r;
}


float getBox(vec3 p, float size) {
    p = abs(p) - size;
    return max(p.x, max(p.y, p.z));
}


float getCross(vec3 p, float size) {
    p = abs(p) - size / 3.0;
    float bx = max(p.y, p.z);
    float by = max(p.x, p.z);
    float bz = max(p.x, p.y);
    return min(min(bx, by), bz);
}


float getInnerMenger(vec3 p, float size) {
    float d = EPSILON;
    float scale = 1.0;
    for (int i = 0; i < 4; i++) {
        float r = size / scale;
//        vec3 q = mod(p + r, 2.0 * r) - r;
        vec3 q = mod(p * (i + 1.0 * scale) / (2.0 * scale) + r, 2.0 * r) - r;
        d = min(d, getCross(q, r));
        scale *= 3.0;
    }
    return d;
}


vec3 hash33(vec3 p) {
	p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.xxy + p.yxx) * p.zyx);
}


float hash13(vec3 p) {
	p = fract(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return fract((p.x + p.y) * p.z);
}


vec4 map(vec3 p) {
    float d = 0.0;
    float size = 0.5;
    vec3 col = vec3(1);

    p.z += u_time * 0.3;
    p.xy *= rot(u_time * 0.1);

    d = -getInnerMenger(p, size);

//    col = abs(floor(p * 6.0 * size - size) + 0.1);
    col = hash33(floor(p * 3.0 * size - size) + 2e-5 * u_time);
//    col = vec3(hash13(floor(p * 3.0 * size - 1.0 * size)));
    return vec4(col, d * 0.9);
}


vec4 rayMarch(vec3 ro, vec3 rd, int steps) {
    float dist; vec3 p; vec3 col;
    for (int i; i < steps; i++) {
        p = ro + rd * dist;
        vec4 res = map(p);
        col = res.rgb;
        if (res.w < EPSILON) break;
        dist += res.w;
        if (dist > MAX_DIST) break;
    }
    return vec4(col, dist);
}


float getAO(vec3 pos, vec3 norm) {
    float AO_SAMPLES = 10.0;
    float AO_FACTOR = 1.0;
    float result = 1.0;
    float s = -AO_SAMPLES;
    float unit = 1.0 / AO_SAMPLES;
    for (float i = unit; i < 1.0; i += unit) {
        result -= pow(1.6, i * s) * (i - map(pos + i * norm).w);
    }
    return result * AO_FACTOR;
}


vec3 getNormal(vec3 p) {
    vec2 e = vec2(EPSILON, 0.0);
    vec3 n = map(p).w - vec3(map(p - e.xyy).w, map(p - e.yxy).w, map(p - e.yyx).w);
    return normalize(n);
}


vec3 render(vec2 uv) {
    vec3 col = vec3(0);
    vec3 ro = vec3(0, 0, -1.9);
    vec3 rd = normalize(vec3(uv, 2.0));

    mat2 rm = rot(PI * 0.5 + u_time * 0.25);
    rd.xy *= rm;
    rd.xz *= rm;

    vec4 res = rayMarch(ro, rd, MAX_STEPS);

    if (res.w < MAX_DIST) {
        vec3 p = ro + rd * res.w;
        vec3 normal = getNormal(p);

        // shading
        float diff = 0.7 * max(0.0, dot(normal, -rd));
        vec3 ref = reflect(rd, normal);
//        float spec = max(0.0, pow(dot(ref, -rd), 32.0));
        float ao = getAO(p, normal);
//        col += (spec + diff) * ao * res.rgb;

        // reflections
        vec3 ref_col;
        vec4 ref_res = rayMarch(p + normal * 0.05, ref, 15);
        vec3 ref_p = p + ref * ref_res.w;
        vec3 ref_normal = getNormal(ref_p);
        ref_col = ref_res.rgb * max(0.0, dot(-ref, ref_normal));

//        col = 0.1 * ref_col + 0.9 * col;
        col = ref_col * ao;

        // fog
        vec3 c = abs(floor(p + 6.0));
        col = mix(c * 0.4, col, exp(-0.03 * res.w * res.w));
    }
    return col;
}



void main() {
    vec2 uv = 2.0 * gl_FragCoord.xy - u_resolution.xy;
    uv /= u_resolution.y;
    vec3 col = render(uv);

    fragColor = vec4(sqrt(col), 1.0);
}
)";
// Compile shaders and link them into a program
GLuint compileShaders(const char* vertexSource, const char* fragmentSource);

int main() {
    // Initialize GLFW and create a window
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(RESOLUTION_W, RESOLUTION_H, "Infinite Menger Sponge", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Compile shaders
    GLuint shaderProgram = compileShaders(vertexShaderSource, fragmentShaderSource);

    // Setup fullscreen quad
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Set viewport and clear the screen
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // Pass uniforms
        glUniform1f(glGetUniformLocation(shaderProgram, "u_time"), (float)glfwGetTime());
        glUniform2f(glGetUniformLocation(shaderProgram, "u_resolution"), (float)width, (float)height);

        // Draw the quad
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void checkCompileErrors(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Shader compilation error in " << type << " shader: " << infoLog << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Program linking error: " << infoLog << std::endl;
        }
    }
}

GLuint compileShaders(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    checkCompileErrors(vertexShader, "VERTEX");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    checkCompileErrors(fragmentShader, "FRAGMENT");

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkCompileErrors(shaderProgram, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
