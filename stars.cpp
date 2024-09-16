#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdlib>
#include <ctime>

// Window dimensions
const GLint WIDTH = 1080, HEIGHT = 1080;

// Number of stars
const int numStars = 1000;
const int trailLength = 10; // Length of the trail

// Star structure
struct Star {
    float x, y, z;  // Current position in 3D space
    std::vector<std::pair<float, float>> trail; // Previous positions for the trail
    bool reset; // Flag to indicate whether the star was just reset
};

// Starfield
std::vector<Star> stars;
float dspeed = 0.01f;
float speed = 0.0f;
// Initialize stars with random positions
void initStars() {
    for (int i = 0; i < numStars; ++i) {
        Star star;
        star.x = static_cast<float>(rand() % WIDTH) - WIDTH / 2.0f;
        star.y = static_cast<float>(rand() % HEIGHT) - HEIGHT / 2.0f;
        star.z = static_cast<float>(rand() % 2000) + 1.0f;
        star.trail = std::vector<std::pair<float, float>>(trailLength, {star.x, star.y});
        star.reset = false;
        stars.push_back(star);
    }
}

// Render stars and trails
void renderStars() {
    for (const Star& star : stars) {
        // Only draw the trail if it has more than one unique point and was not just reset
        if (!star.reset) {
            // Draw the trail
            glBegin(GL_LINES);
            for (size_t i = 1; i < star.trail.size(); ++i) { // Start from 1 to draw segments
                // Fade the trail segments
                float brightness = 1.0f - (i / static_cast<float>(star.trail.size()));
                glColor3f(brightness, brightness, brightness*2);
                // Draw the segment
                glVertex2f(star.trail[i - 1].first, star.trail[i - 1].second);
                glVertex2f(star.trail[i].first, star.trail[i].second);
            }
            glEnd();
        }

        // Draw the star
        glBegin(GL_POINTS);
        // Perspective scaling: stars closer to the camera are larger and brighter
        float brightness = 1.0f - (star.z / 2000.0f);
        float size = 5.0f * (1.0f - (star.z / 2000.0f)); // Adjust the size based on depth
        glPointSize(size < 1.0f ? 1.0f : size); // Ensure the size is at least 1.0
        glColor3f(brightness, brightness, brightness);

        // Transform star position to screen coordinates with perspective effect
        float screenX = (star.x / star.z) * WIDTH + WIDTH / 2.0f;
        float screenY = (star.y / star.z) * HEIGHT + HEIGHT / 2.0f;
        glVertex2f(screenX, screenY);
        glEnd();
    }
}

// Update stars to simulate movement
void updateStars() {
    if(speed < 20.0f)
        speed += dspeed;
    for (Star& star : stars) {
        // Move the star towards the camera
        star.z -= speed; // Adjust speed to make the depth effect more apparent

        // Transform star position to screen coordinates
        float screenX = (star.x / star.z) * WIDTH + WIDTH / 2.0f;
        float screenY = (star.y / star.z) * HEIGHT + HEIGHT / 2.0f;

        // Update the trail if the star is not resetting
        if (star.z > 0.0f) {
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
            star.z = static_cast<float>(rand() % 2000) + 1.0f;

            // Clear the trail and reinitialize to the new starting position
            star.trail.clear();
            // Initialize the trail at the new starting position
            float newScreenX = (star.x / star.z) * WIDTH + WIDTH / 2.0f;
            float newScreenY = (star.y / star.z) * HEIGHT + HEIGHT / 2.0f;
            star.trail = std::vector<std::pair<float, float>>(trailLength, {newScreenX, newScreenY});

            // Set reset flag to true to skip drawing this frame
            star.reset = true;
        } else {
            // Reset flag is turned off when the star has moved at least once after reset
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
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Space Warp Effect with Enhanced Depth", nullptr, nullptr);
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

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Update and render stars
        updateStars();
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
