#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

// Window dimensions
const GLint WIDTH = 1080, HEIGHT = 1080;

// Number of stars
const int numStars = 2000;
const int trailLength = 10; // Length of the trail

// Sway parameters
float timeElapsed = 0.0f;
const float swayAmplitude = 100.0f; // Controls how much the stars sway left and right
const float swayFrequency = 0.5f;   // Controls how fast the sway oscillates

// Star structure
struct Star {
    float x, y, z;  // Current position in 3D space
    std::vector<std::pair<float, float>> trail; // Previous positions for the trail
    bool reset; // Flag to indicate whether the star was just reset
};

// Starfield
std::vector<Star> stars;

// Initialize stars with random positions
void initStars() {
    for (int i = 0; i < numStars; ++i) {
        Star star;
        star.x = static_cast<float>(rand() % WIDTH) - WIDTH / 2.0f;
        star.y = static_cast<float>(rand() % HEIGHT) - HEIGHT / 2.0f;
        star.z = static_cast<float>(rand() % 1000) + 1.0f;
        star.trail = std::vector<std::pair<float, float>>(trailLength, {star.x, star.y});
        star.reset = true; // Set reset to true initially to avoid drawing the trail right away
        stars.push_back(star);
    }
}

// Render stars and trails
void renderStars() {
    for (const Star& star : stars) {
        // Calculate the sway offset based on the current time
        float swayOffset = swayAmplitude * sin(timeElapsed * swayFrequency);

        // Only draw the trail if the star was not just reset
        if (!star.reset) {
            // Draw the trail
            glBegin(GL_LINES);
            for (size_t i = 1; i < star.trail.size(); ++i) { // Start from 1 to draw segments
                // Fade the trail segments
                float brightness = 1.0f - (i / static_cast<float>(star.trail.size()));
                glColor3f(brightness, brightness, brightness*2);
                // Draw the segment with sway offset applied
                float trailX1 = star.trail[i - 1].first + swayOffset;
                float trailX2 = star.trail[i].first + swayOffset;
                glVertex2f(trailX1, star.trail[i - 1].second);
                glVertex2f(trailX2, star.trail[i].second);
            }
            glEnd();
        }

        // Draw the star
        glBegin(GL_POINTS);
        // Perspective scaling: stars closer to the camera are larger and brighter
        float brightness = 1.0f - (star.z / 1000.0f);
        float size = 5.0f * (1.0f - (star.z / 1000.0f)); // Adjust the size based on depth
        glPointSize(size < 1.0f ? 1.0f : size); // Ensure the size is at least 1.0
        glColor3f(brightness, brightness, brightness);

        // Apply the sway effect to the star's x-coordinate
        float screenX = ((star.x + swayOffset) / star.z) * WIDTH + WIDTH / 2.0f;
        float screenY = (star.y / star.z) * HEIGHT + HEIGHT / 2.0f;
        glVertex2f(screenX, screenY);
        glEnd();
    }
}

// Update stars to simulate movement
void updateStars(float deltaTime) {
    // Update the time for the sway effect
    timeElapsed += deltaTime;

    for (Star& star : stars) {
        // Move the star towards the camera
        star.z -= 20.1f; // Adjust speed to make the depth effect more apparent

        // Transform star position to screen coordinates without sway offset
        float screenX = (star.x / star.z) * WIDTH + WIDTH / 2.0f;
        float screenY = (star.y / star.z) * HEIGHT + HEIGHT / 2.0f;

        // If the star is not resetting, update the trail
        if (!star.reset) {
            // Shift the previous positions to create the trail effect
            for (int i = trailLength - 1; i > 0; --i) {
                star.trail[i] = star.trail[i - 1];
            }
            star.trail[0] = {screenX, screenY};
        }

        // If the star is too close, reset it to the back
        if (star.z <= 0.0f) {
            star.x = static_cast<float>(rand() % WIDTH) - WIDTH / 2.0f;
            star.y = static_cast<float>(rand() % HEIGHT) - HEIGHT / 2.0f;
            star.z = static_cast<float>(rand() % 1000) + 1.0f;

            // Clear the trail and reinitialize to the new starting position
            star.trail.clear();
            // Initialize the trail at the new starting position without sway offset
            float newScreenX = (star.x / star.z) * WIDTH + WIDTH / 2.0f;
            float newScreenY = (star.y / star.z) * HEIGHT + HEIGHT / 2.0f;
            star.trail = std::vector<std::pair<float, float>>(trailLength, {newScreenX, newScreenY});

            // Set reset flag to true to avoid drawing the trail immediately
            star.reset = true;
        } else if (star.reset) {
            // If the star was reset and has moved forward enough, allow the trail to start updating
            star.reset = false;
        }
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;
    }

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Space Warp Effect with Proper Sway", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's OpenGL context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        return -1;
    }

    // Set up OpenGL options
    glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);

    // Initialize stars
    srand(static_cast<unsigned int>(time(0)));
    initStars();

    // Timing variables
    float lastTime = glfwGetTime();
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate deltaTime
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Update and render stars
        updateStars(deltaTime);
        renderStars();

        // Swap buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Clean up and exit
    glfwTerminate();
    return 0;
}
