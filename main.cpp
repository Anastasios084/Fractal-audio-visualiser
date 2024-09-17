#include <GL/glew.h> // For OpenGL functions
#include <GLFW/glfw3.h> // For GLFW window and input handling
#include <iostream>
#include "audioAnalyzer.h"
#include <ctime>    // For time()
#include <thread>
#include <chrono>


#define RESOLUTION 1080
#define RESOLUTION_F 1080.0f

// Vertex Shader
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 position;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

// Function to be executed by the thread
void threadFunction(audioAnalyzer* anal) {
    while(true){
        anal->init();
        anal->startSession(30);
    }
}
const char* fragmentShaderSource2 = R"(
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
    vec2 coord = (gl_FragCoord.xy / u_resolution - 0.5) * u_zoom + u_offset;

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
        //color += vec3(t * 9.0 * c_parameter_r, t * t * 15.0 * c_parameter_g, t * t * t * 8.5 * c_parameter_b);

        color += vec3(t * 9.0 * c_parameter_r/2, t * 9.0 * c_parameter_g/2, t * 9.0 * c_parameter_b/2);
        if (iterations == MAX_ITER) color += vec3(0.0);
    }

    // Average the color
    color /= float(NUM_SAMPLES);

    FragColor = vec4(color, 1.0);
}
)";


// Check for shader compile errors
void checkShaderCompileError(GLuint shader) {
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}

// Check for shader program link errors
void checkProgramLinkError(GLuint program) {
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
}

float lerp(float start, float end, float t) {
    return start + t * (end - start);
}

// Call this whenever you want a random float between -0.2 and 0.2
float getRandomFloat()
{
    // Generate a random float between 0 and 1
    float random = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);

    // Scale and shift to the desired range [-0.2, 0.2]
    return random * 0.4f - 0.2f;
}

// Call this once at the start of your program
void initializeRandomSeed()
{
    std::srand(static_cast<unsigned int>(std::time(0)));
}
int main() {
    //INITIALIZE MUSIC ANALYZER
    audioAnalyzer anal;

    std::thread myThread(threadFunction, &anal);

    float bpm = anal.getCurrentBPM();//detect_shouldReturnTheBpmAndTheBeat("./mangalam.mp3", PcmAudioFrameFormat::Float);
    // return 0;
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set OpenGL version (3.3)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Enable multisampling
    // glEnable(GL_MULTISAMPLE);

    // Request a multisampled framebuffer with 16 samples
    glfwWindowHint(GLFW_SAMPLES, 16);  // Use GLFW for window creation

    // Create window
    GLFWwindow* window = glfwCreateWindow(RESOLUTION, RESOLUTION, "Mandelbrot Fractal", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW after creating OpenGL context
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Enable multisampling after context creation
    glEnable(GL_MULTISAMPLE);

        // Check the number of samples actually being used
    int samples;
    glGetIntegerv(GL_SAMPLES, &samples);
    std::cout << "Using " << samples << " samples for multisampling." << std::endl;


    // Vertex data for a full-screen quad
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // Create vertex array and buffers
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO); // Correct usage with argument for the VAO id
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkShaderCompileError(vertexShader);

    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource2, NULL);
    glCompileShader(fragmentShader);
    checkShaderCompileError(fragmentShader);

    // Link shaders to create a program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkProgramLinkError(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Use the shader program
    glUseProgram(shaderProgram);

// Define the complex constant 'c' for the Julia set
float cX = -0.7f;  // Real part of c
float cY = 0.27015f;  // Imaginary part of c

float zoom = 1.0f;    // Initial zoom level
float offsetX = 0.0f, offsetY = 0.0f;  // Initial offset
float zoomSpeed = 0.9995f;  // Slow zoom speed (slightly less than 1 means zooming in)
float r = 1.0f;
float g = 1.0f;
float b = 1.0f;

bool increase, decrease;
increase = false;
decrease = true;
float threshold_color = 3.0f;

float change = 0.0001f;

// Initialize variables
float lastTime = glfwGetTime();
float elapsedTime = 0.0f;
// Define start and end values for the lerp
float startValueCX = cX;
float endValueCX = cX-change;

float startValueCY = cY;
float endValueCY = cY-change;

// RED LERP
float startValueR = r;
float endValueR= r-getRandomFloat();
// GREEN LERP
float startValueG = g;
float endValueG= g-getRandomFloat();
// BLUE LERP
float startValueB = b;
float endValueB= b-getRandomFloat();


float durationBeat = 60.0f / bpm; // Duration of one beat in seconds
auto startTime = std::chrono::steady_clock::now();
while (!glfwWindowShouldClose(window)) {
    // std::cout << r << ", " << g << ", " << b << std::endl;
    
    
    // Inside your render loop
    float currentTime = glfwGetTime();
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    // Update elapsed time
    elapsedTime += deltaTime;
    // Calculate the interpolation parameter t
    float t = elapsedTime / durationBeat;
    t = t > 1.0f ? 1.0f : t; // Clamp t to a maximum of 1.0f


    // Automatically zoom in slowly
    zoom *= zoomSpeed;
    if(cX > -0.802f && decrease){
        // Perform the interpolation
        float currentValueX = lerp(startValueCX, endValueCX, t);
        float currentValueY = lerp(startValueCY, endValueCY, t);
        cX = currentValueX;
        startValueCX = cX;
        endValueCX = cX-change;

        cY = currentValueY;
        startValueCY = cY;
        endValueCY = cY-change;
        // cY -= 0.0001f*zoom;
    }else{
        float currentValueX = lerp(startValueCX, endValueCX, t);
        float currentValueY = lerp(startValueCY, endValueCY, t);
        cX = currentValueX;
        startValueCX = cX;
        endValueCX = cX+change;

        cY = currentValueY;
        startValueCY = cY;
        endValueCY = cY+change;
        decrease = false;
        // cX += 0.0003f*zoom;
        // cY += 0.0003f*zoom;

        if(cX > -0.699f){
            decrease = true;
        }
    }

    float currentValueR = lerp(startValueR, endValueR, t);
    r = currentValueR;
    startValueR = r;
    endValueR = r+getRandomFloat();

    float currentValueG = lerp(startValueG, endValueG, t);
    g = currentValueG;
    startValueG = g;
    endValueG = g+getRandomFloat();

    float currentValueB = lerp(startValueB, endValueB, t);
    b = currentValueB;
    startValueB = b;
    endValueB = b+getRandomFloat();

    // Reset if the beat is complete to repeat the animation
    if (elapsedTime >= durationBeat) {
        elapsedTime = 0.0f;
    }
    // Input handling (for panning only)
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) offsetY += 0.05f * zoom;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) offsetY -= 0.05f * zoom;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) offsetX -= 0.05f * zoom;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) offsetX += 0.05f * zoom;

        // Change the real part of 'c' using Q and W keys
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cX += 0.00001f*zoom;  // Increase real part of c
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cX -= 0.00001f*zoom;  // Decrease real part of c

    // Optional: You can also add controls for the imaginary part (e.g., A and D)
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cY += 0.00001f*zoom;  // Increase imaginary part of c
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cY -= 0.00001f*zoom;  // Decrease imaginary part of c


    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) r += 0.1f;  // Increase imaginary part of c
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) g += 0.1f;  // Decrease imaginary part of c
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) b += 0.1f;  // Increase imaginary part of c

    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) r -= 0.1f;  // Increase imaginary part of c
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) g -= 0.1f;  // Decrease imaginary part of c
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) b -= 0.1f;  // Increase imaginary part of c

    if(r > threshold_color){
        r = threshold_color;
    }else if(r < 0.2f){
        r = 0.2f;
    }
    
    if(g > threshold_color){
        g = threshold_color;
    }else if(g < 0.2f){
        g = 0.2f;
    }

    if(b > threshold_color){
        b = threshold_color;
    }else if(b < 0.2f){
        b = 0.2f;
    }
    // Send uniform values to the shader
    glUniform2f(glGetUniformLocation(shaderProgram, "u_resolution"), RESOLUTION_F, RESOLUTION_F);
    glUniform2f(glGetUniformLocation(shaderProgram, "u_offset"), offsetX, offsetY);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_zoom"), zoom);
    glUniform2f(glGetUniformLocation(shaderProgram, "u_c"), cX, cY);  // Send the complex constant c
    glUniform1f(glGetUniformLocation(shaderProgram, "c_parameter_r"), r);  // red
    glUniform1f(glGetUniformLocation(shaderProgram, "c_parameter_g"), g);  // red
    glUniform1f(glGetUniformLocation(shaderProgram, "c_parameter_b"), b);  // red

    // Clear the screen and draw the quad
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Swap buffers and poll for events
    glfwSwapBuffers(window);
    glfwPollEvents();
     // Convert float seconds to a duration
    
    // Get the start time
    std::chrono::duration<float> duration(durationBeat);

    if(std::chrono::steady_clock::now() - startTime < duration){
        startTime = std::chrono::steady_clock::now();
        bpm = anal.getCurrentBPM();//detect_shouldReturnTheBpmAndTheBeat("./mangalam.mp3", PcmAudioFrameFormat::Float);
        if((int)bpm <= 0){ // initialize to a safe value until valid bpm value
            bpm = 120.0;
        }

        durationBeat = 60.0f / bpm; // Duration of one beat in seconds
        cout << (int)bpm << endl;
    }

}

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
