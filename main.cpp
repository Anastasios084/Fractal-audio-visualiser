#include <GL/glew.h> // For OpenGL functions
#include <GLFW/glfw3.h> // For GLFW window and input handling
#include <iostream>
#include "audioAnalyzer.h"
#include <ctime>    // For time()
#include <thread>
#include <chrono>
#include "juliaChill.h"
#include "juliaTrippy.h"
#include "juliaNoisy.h"
#include "juliaDark.h"

#define optionShader 1

#define RESOLUTION_W 2560
#define RESOLUTION_H 1080
#define RESOLUTION_F 1920.0f
const float swayAmplitude = 100.0f; // Controls how much the stars sway left and right
const float swayFrequency = 0.5f;   // Controls how fast the sway oscillates

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
    return random * 0.4f;
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
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // Disable window decorations (borderless)
    // Set OpenGL version (3.3)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    // Enable multisampling
    // glEnable(GL_MULTISAMPLE);

    // Request a multisampled framebuffer with 16 samples
    //glfwWindowHint(GLFW_SAMPLES, 8);  // Use GLFW for window creation
    //glEnable(GL_MULTISAMPLE); // Enable MSAA in OpenGL
    // Create window
    GLFWwindow* window = glfwCreateWindow(RESOLUTION_W, RESOLUTION_H, "Mandelbrot Fractal", NULL, NULL);
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
    glShaderSource(fragmentShader, 1, &fragmentShaderSource2, NULL); /////////////////////////////////////////////////////// CHANGE HERE FOR DIFFERENT SHADER
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
    float cX = -0.76f;  // Real part of c
    float cY = 0.27015f;  // Imaginary part of c

    float zoom = 0.6f;    // Initial zoom level
    float offsetX = -0.5f, offsetY = 0.0f;  // Initial offset
    float zoomSpeed = 0.999995f;  // Slow zoom speed (slightly less than 1 means zooming in)
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;

    bool increase, decreaseX, decreaseY, decrease;
    increase = false;
    decreaseX = true;
    decreaseY = true;
    decrease = true;
    float threshold_color = 3.0f;

    float change = 0.0002f;

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

    float amp = sin(anal.getCurrentFrequency() * swayAmplitude) * sin(anal.getCurrentFrequency() * swayAmplitude);
    float startAmp = b;
    float endAmp= amp+getRandomFloat();

    float durationBeat = 60.0f / bpm; // Duration of one beat in seconds
    auto startTime = std::chrono::steady_clock::now();
    int counter = 0;

    cout << "amp -> " << amp << endl;
    cout << anal.getCurrentFrequency() << " - " << anal.maxLowBeat() << (anal.getCurrentFrequency()/anal.maxLowBeat()) << endl;
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
        cout << change << endl;

    // if(cX > -0.77f && decrease){ // shader chill
    if(cX > -0.77f && decrease){
        // Perform the interpolation
        float currentValueX = lerp(startValueCX, endValueCX, t);
        cX = currentValueX;
        startValueCX = cX;
        endValueCX = cX-abs(change);
        // cY -= 0.0001f*zoom;
    }else{
        float currentValueX = lerp(startValueCX, endValueCX, t);
        cX = currentValueX;
        startValueCX = cX;
        endValueCX = cX+abs(change);

        decrease = false;
        // cX += 0.0003f*zoom;
        // cY += 0.0003f*zoom;

        // if(cX > -0.7f){
        if(cX > -0.69f){
            decrease = true;
        }
    }


    // if(cY > 0.259 && decreaseY){
    if(cY > 0.259 && decreaseY){
        // Perform the interpolation
        float currentValueY = lerp(startValueCY, endValueCY, t);
        cY = currentValueY;
        startValueCY = cY;
        endValueCY = cY-abs(change);
        // cY -= 0.0001f*zoom;
    }else{
        float currentValueY = lerp(startValueCY, endValueCY, t);
        cY = currentValueY;
        startValueCY = cY;
        endValueCY = cY+abs(change);
        decreaseY = false;
        // cX += 0.0003f*zoom;
        // cY += 0.0003f*zoom;

        // if(cY > 0.273){
        if(cY > 0.28){
            decreaseY = true;
        }
    }

        // float currentValueX = lerp(startValueCX, endValueCX, t);
        // cX = currentValueX;
        // startValueCX = cX;
        // endValueCX = cX+change;

        // if(cX > -0.68f){
        //     // cX = -0.68;
        //     endValueCX = cX-abs(change);
        //     // change = -abs(change);
        // }else if(cX < -0.72f){
        //     // cX = -0.72;
        //     endValueCX = cX+abs(change);
        //     // change = abs(change);
        // }

        // if(cY > 0.252f && decreaseY){
        //     float currentValueY = lerp(startValueCY, endValueCY, t);
        //     cY = currentValueY;
        //     startValueCY = cY;
        //     endValueCY = cY-change;
        // }else{
        //     float currentValueY = lerp(startValueCY, endValueCY, t);
        //     cY = currentValueY;
        //     startValueCY = cY;
        //     endValueCY = cY+change;

        //     decreaseY = false;
        //     if(cY > 0.28f){
        //         decreaseY = true;
        //     }

        // }

        // float currentValueY = lerp(startValueCY, endValueCY, t);
        // cY = currentValueY;
        // startValueCY = cY;
        // endValueCY = cY+change;

        // if(cY > 0.28f){
        //     endValueCY = cY-abs(change);
        //     // change = -abs(change);

        // }else if(cY < 0.252f){
        //     cY = 0.252f;
        //     endValueCY = cY+abs(change);
        //     // change = abs(change);

        // }




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

        // if(r > threshold_color){
        //     r = threshold_color;
        // }else if(r < 0.0f){
        //     r = 0.5f;
        // }
        
        // if(g > threshold_color){
        //     g = threshold_color;
        // }else if(g < 0.0f){
        //     g = 0.0f;
        // }
        
        // if(b > threshold_color){
        //     b = threshold_color;
        // }else if(b < 0.0f){
        //     b = 0.5f;
        // }

        // Send uniform values to the shader
        glUniform2f(glGetUniformLocation(shaderProgram, "u_resolution"), RESOLUTION_W, RESOLUTION_H);
        glUniform2f(glGetUniformLocation(shaderProgram, "u_offset"), offsetX, offsetY);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_zoom"), zoom);
        glUniform2f(glGetUniformLocation(shaderProgram, "u_c"), cX, cY);  // Send the complex constant c
        glUniform1f(glGetUniformLocation(shaderProgram, "c_parameter_r"), r);  // red
        glUniform1f(glGetUniformLocation(shaderProgram, "c_parameter_g"), g);  // red
        glUniform1f(glGetUniformLocation(shaderProgram, "c_parameter_b"), b);  // red
        glUniform1f(glGetUniformLocation(shaderProgram, "amplitude"), amp);  // red

        // Clear the screen and draw the quad
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Swap buffers and poll for events
        glfwSwapBuffers(window);
        glfwPollEvents();
        // Convert float seconds to a duration
        

        // float currentAmp= lerp(startAmp, endAmp, t);
        // amp = currentAmp;
        // startAmp = amp;
        // endAmp= sin(anal.getCurrentFrequency() * swayAmplitude) * sin(anal.getCurrentFrequency() * swayAmplitude);

        // Get the start time
        std::chrono::duration<float> duration(durationBeat);
        if(std::chrono::steady_clock::now() - startTime < duration){
            startTime = std::chrono::steady_clock::now();
            bpm = anal.getCurrentBPM();//detect_shouldReturnTheBpmAndTheBeat("./mangalam.mp3", PcmAudioFrameFormat::Float);
            if((int)bpm <= 0){ // initialize to a safe value until valid bpm value
                bpm = 120.0;
            }

            float currentAmp= lerp(startAmp, endAmp, t);
            amp = currentAmp;
            startAmp = amp;
            endAmp= sin(anal.getCurrentFrequency() * swayAmplitude) * sin(anal.getCurrentFrequency() * swayAmplitude);

            // startAmp = currentAmp;
            // endAmp = anal.getCurrentFrequency();
            // currentAmp= lerp(startAmp, endAmp, t);
            // amp = currentAmp;
            // startAmp = amp;
            // endAmp= sin(anal.getCurrentFrequency() * swayAmplitude) * sin(anal.getCurrentFrequency() * swayAmplitude);

            cout << "amp -> " << amp << endl;
            cout << anal.getCurrentFrequency() << " - " << anal.maxLowBeat() << (anal.getCurrentFrequency()/anal.maxLowBeat()) << endl;

            durationBeat = 60.0f / bpm; // Duration of one beat in seconds
            cout << (int)bpm << endl;
            if(counter == 20){
                change = (anal.getCurrentFrequency()/anal.maxLowBeat()) > 0.0002 ? 0.0002 : (anal.getCurrentFrequency()/anal.maxLowBeat());
                if(change > 0.0002){
                    change = 0.0002;
                }else if (change < -0.0002){
                    change = -0.0002;
                }else if((int)change < -10){
                    change = 0.0002;
                }
                counter = 0;
            }else{
                counter += 1;
            }



            if(anal.lowBeat()){
                
                r = r > threshold_color ? threshold_color + getRandomFloat() : r + amp * sin(M_PI*r + getRandomFloat()*100);
                g = g > threshold_color ? threshold_color + getRandomFloat() : g + amp * sin(M_PI*g + getRandomFloat()*100);
                b = b > threshold_color ? threshold_color + getRandomFloat() : b + amp * sin(M_PI*b + getRandomFloat()*100);

                anal.setLowBeat(false);
            }else{
                if(r>0.05)
                    r -= 0.01+getRandomFloat()/100;
                else
                    r = 0.05;
                if(g>0.0f)
                    g -= 0.01+getRandomFloat()/100;
                else
                    g = 0.0;
                if(b>0.05)
                    b -= 0.01+getRandomFloat()/100;
                else
                    b = 0.5;  
            }
            cout << "R: " << r << " - G: " << g << " - B: " << b << endl;

            
        }
        cout << cX << " - " << cY << endl;
    }

        // Cleanup
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteProgram(shaderProgram);

        glfwTerminate();
        return 0;
}
