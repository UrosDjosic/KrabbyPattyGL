//Main.cpp
//SV20/2022
//Updated to 3D with 2D menu and ingredient stacking

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../Header/Util.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

// Camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, -10.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.3f;
float yaw = 90.0f;  // Changed to face forward initially
float pitch = -20.0f;  // Look slightly down
bool firstMouse = true;
double lastX = 400.0, lastY = 300.0;

// Light variables
glm::vec3 lightPos = glm::vec3(0.0f, 10.0f, 0.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
bool lightEnabled = true;

// Patty position (now in 3D space)
glm::vec3 pattyPos = glm::vec3(-5.0f, 5.0f, 0.0f);
float pattyMoveSpeed = 0.1f;

// Models
Model grillModel;
Model pattyModel;

Model tableModel;
Model plateModel;

Model bunBottomModel;
Model cookedPattyModel;
Model ketchupModel;
Model mustardModel;
Model picklesModel;
Model onionModel;
Model lettuceModel;
Model cheeseModel;
Model tomatoModel;
Model bunTopModel;

struct Rect { float left, right, top, bottom; };

std::string activeIngredient = "bunBottom"; //first ingredient

struct StickyOverlay {
    unsigned tex;
    unsigned vao;
    float hShift;
    float vShift;
};
std::vector<StickyOverlay> stainsAndSuccesses;

GLFWcursor* cursor;

unsigned bg;
unsigned floorTex;
unsigned credentials;
unsigned button;
float buttonLeft = -0.2f;
float buttonRight = 0.2f;
float buttonTop = 0.2f;
float buttonBottom = -0.2f;
bool buttonClicked = false;

unsigned pozdrav;
float pozdravLeft = -0.3f;
float pozdravRight = 0.3f;
float pozdravTop = 0.8f;
float pozdravBottom = 0.5f;

unsigned ketchupStain;
unsigned mustardStain;

float cookedness = 0.0f;
float finalCookedness = 0.0f; // Store final cookedness value
float shiftHorizontal = 0.0f;
float shiftVertical = 0.0f;

bool fired = false;
bool ketchupDone = false;
bool mustardDone = false;

unsigned barBgVAO; unsigned barBgVBO;
unsigned barFillVAO; unsigned barFillVBO;

// Positioned in the upper left
float barLeft = -0.95f;
float barRight = -0.55f;
float barTop = 0.95f;
float barBottom = 0.85f;

// Grill logical transform (used for both rendering and collision)
glm::vec3 grillPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec2 grillHalfSize = glm::vec2(1.5f, 1.5f); // X and Z half extents

// plating and ingredients
glm::vec3 platePos = glm::vec3(-5.0f, 5.0f, 0.0f); // same spawn as patty
float plateMoveSpeed = 0.1f;

bool platePlaced = false;
glm::vec3 fixedPlatePos;

bool gameCompleted = false; // Flag for when burger is finished

glm::vec3 ingredientPos = glm::vec3(-5.0f, 10.0f, 0.0f); //every ingredient starts 5 units higher
float ingredientSpeed = 0.1f;

bool ingredientPlaced = false; //switches back and forth until bun_top is finally placed
glm::vec3 fixedIngredientPos; //each placed ingredient rewrites it once placed on top of the previous

// Stain tracking
struct Stain {
    std::string type; // "ketchup" or "mustard"
    glm::vec3 position;
    bool onPlate; // true if on plate/burger, false if on floor
};
std::vector<Stain> stains;


int endProgram(const char* msg) {
    std::cout << msg << std::endl;
    glfwTerminate();
    return -1;
}

void mouse_motion_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!buttonClicked) return; // Only rotate camera in 3D mode

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Constrain pitch
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Update camera front vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!buttonClicked) return; // Only process these keys after entering 3D mode

    // Camera movement (Arrow keys)
    if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos += cameraSpeed * cameraFront;
    if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos -= cameraSpeed * cameraFront;
    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    // Light toggle
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
        lightEnabled = !lightEnabled;

    // Patty movement (WASD for X/Z, Shift/Space for Y) - only when cooking
    if (cookedness < 1.0f) {
        if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
            pattyPos.x += pattyMoveSpeed;
        if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
            pattyPos.x -= pattyMoveSpeed;
        if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
            pattyPos.z += pattyMoveSpeed;
        if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
            pattyPos.z -= pattyMoveSpeed;
        if (key == GLFW_KEY_LEFT_SHIFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
            pattyPos.y -= pattyMoveSpeed;
        if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
            pattyPos.y += pattyMoveSpeed;
    }

    else if (cookedness >= 2.0f && !platePlaced) {
        if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
            platePos.x += plateMoveSpeed;
        if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
            platePos.x -= plateMoveSpeed;
        if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
            platePos.z += plateMoveSpeed;
        if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
            platePos.z -= plateMoveSpeed;
        if (key == GLFW_KEY_LEFT_SHIFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
            platePos.y -= plateMoveSpeed;
        if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
            platePos.y += plateMoveSpeed;
    }

    else if (platePlaced) {
        if (activeIngredient == "ketchup" || activeIngredient == "mustard") {
            if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.x += ingredientSpeed;
            if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.x -= ingredientSpeed;
            if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.z += ingredientSpeed;
            if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.z -= ingredientSpeed;
            if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS))
                fired = true; //in main, calculate if firing happened above the previous ingredient (correct) or if it missed
        }
        else {
            if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.x += ingredientSpeed;
            if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.x -= ingredientSpeed;
            if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.z += ingredientSpeed;
            if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.z -= ingredientSpeed;
            if (key == GLFW_KEY_LEFT_SHIFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.y -= ingredientSpeed;
            if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.y += ingredientSpeed;
        }
    }

}

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
    if (!buttonClicked && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        double x_ndc = (xpos / width) * 2.0 - 1.0;
        double y_ndc = 1.0 - (ypos / height) * 2.0;

        if (x_ndc >= buttonLeft && x_ndc <= buttonRight &&
            y_ndc >= buttonBottom && y_ndc <= buttonTop)
        {
            buttonClicked = true;
            // Hide cursor and enable mouse capture for 3D camera control
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

bool isPattyOnGrill()
{
    bool withinX =
        pattyPos.x >= grillPosition.x - grillHalfSize.x &&
        pattyPos.x <= grillPosition.x + grillHalfSize.x;

    bool withinZ =
        pattyPos.z >= grillPosition.z - grillHalfSize.y &&
        pattyPos.z <= grillPosition.z + grillHalfSize.y;

    bool heightMatch = pattyPos.y >= 3.7f && pattyPos.y <= 4.0;

    return withinX && withinZ && heightMatch;
}


void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void formVAOs(float* vertices, size_t size, unsigned int& VAO) {
    unsigned int VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

// Create a simple 3D cube VAO
unsigned int createCubeVAO() {
    float vertices[] = {
        // Positions          // Normals           // Tex Coords
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        // Left face
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        // Right face
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         // Bottom face
         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
          0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
          0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
          0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

         // Top face
         -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
          0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
          0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
          0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return VAO;
}

// Create a simple ground plane
unsigned int createPlaneVAO() {
    float vertices[] = {
        // Positions          // Normals        // Tex Coords
        -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,  0.0f, 10.0f,
         10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f,
         10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 0.0f,

         10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 0.0f,
        -10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,  0.0f, 10.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return VAO;
}

void renderModel(const Model& model,
    unsigned int shader,
    const glm::mat4& modelMatrix,
    bool isPatty = false,
    float cookedness = 0.0f,
    bool forceColor = false,
    glm::vec3 forcedColor = glm::vec3(1.0f))
{
    glUniformMatrix4fv(glGetUniformLocation(shader, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

    if (isPatty) {
        // Use cookedness-based coloring for patty
        glUniform1i(glGetUniformLocation(shader, "uUseCooked"), GL_TRUE);
        glUniform1f(glGetUniformLocation(shader, "uCookedness"), cookedness);
        glUniform1i(glGetUniformLocation(shader, "uUseMaterial"), GL_FALSE);
    }
    else {
        glUniform1i(glGetUniformLocation(shader, "uUseCooked"), GL_FALSE);
        glUniform1i(glGetUniformLocation(shader, "uUseMaterial"), GL_TRUE);

        if (forceColor) {
            glUniform3fv(glGetUniformLocation(shader, "uMaterialDiffuse"), 1, glm::value_ptr(forcedColor));
            glUniform3f(glGetUniformLocation(shader, "uMaterialSpecular"), 0.2f, 0.2f, 0.2f);
            glUniform1f(glGetUniformLocation(shader, "uMaterialShininess"), 16.0f);
        }
        else if (!model.materials.empty()) {
            Material mat = model.materials.begin()->second;
            glUniform3fv(glGetUniformLocation(shader, "uMaterialDiffuse"), 1, glm::value_ptr(mat.diffuse));
            glUniform3fv(glGetUniformLocation(shader, "uMaterialSpecular"), 1, glm::value_ptr(mat.specular));
            glUniform1f(glGetUniformLocation(shader, "uMaterialShininess"), mat.shininess);
        }
    }


    glBindVertexArray(model.VAO);
    glDrawArrays(GL_TRIANGLES, 0, model.vertexCount);

    // Reset uniforms
    glUniform1i(glGetUniformLocation(shader, "uUseCooked"), GL_FALSE);
    glUniform1i(glGetUniformLocation(shader, "uUseMaterial"), GL_FALSE);
}

glm::vec3 getIngredientColor(const std::string& ingredient) {
    if (ingredient == "bunBottom" || ingredient == "bunTop") {
        return glm::vec3(0.85f, 0.7f, 0.45f); // Light brown bun
    }
    else if (ingredient == "ketchup") {
        return glm::vec3(0.8f, 0.1f, 0.1f); // Red
    }
    else if (ingredient == "mustard") {
        return glm::vec3(0.95f, 0.85f, 0.1f); // Yellow
    }
    else if (ingredient == "pickles") {
        return glm::vec3(0.3f, 0.6f, 0.2f); // Green
    }
    else if (ingredient == "onion") {
        return glm::vec3(0.95f, 0.9f, 0.85f); // White/cream
    }
    else if (ingredient == "lettuce") {
        return glm::vec3(0.4f, 0.75f, 0.3f); // Bright green
    }
    else if (ingredient == "cheese") {
        return glm::vec3(1.0f, 0.8f, 0.2f); // Orange/yellow
    }
    else if (ingredient == "tomato") {
        return glm::vec3(0.9f, 0.2f, 0.2f); // Red
    }
    return glm::vec3(0.5f, 0.5f, 0.5f); // Default gray
}

int main()
{
    // GLFW initialization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Fullscreen window
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) return endProgram("Glavni monitor nije pronadjen.");

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) return endProgram("Video mod nije pronadjen.");

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Brza Hrana 3D", monitor, NULL);
    if (!window) return endProgram("Prozor nije uspeo da se kreira.");

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetCursorPosCallback(window, mouse_motion_callback);

    cursor = loadImageToCursor("res/cursor.png");
    glfwSetCursor(window, cursor);

    // GLEW initialization
    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Preprocess textures (only the ones we actually use)
    preprocessTexture(bg, "res/background.png");
    preprocessTexture(floorTex, "res/floor.png");
    preprocessTexture(credentials, "res/credentials.png");
    preprocessTexture(button, "res/button.png");
    preprocessTexture(ketchupStain, "res/ketchup_stain.png");
    preprocessTexture(mustardStain, "res/mustard_stain.png");
    preprocessTexture(pozdrav, "res/prijatno.png");

    // 2D menu vertices
    float bgVertices[] = {
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };
    float creditVertices[] = {
         0.5f,  0.6f,  0.0f, 0.0f,
         0.5f,  1.1f,  0.0f, 1.0f,
         1.0f,  1.1f,  1.0f, 1.0f,
         0.5f,  0.6f,   0.0f, 0.0f,
         1.0f,  1.1f,  1.0f, 1.0f,
         1.0f,  0.6f,  1.0f, 0.0f
    };
    float buttonVertices[] = {
        buttonLeft,  buttonBottom,  0.0f, 0.0f,
        buttonRight, buttonBottom,  1.0f, 0.0f,
        buttonRight, buttonTop,     1.0f, 1.0f,
        buttonLeft,  buttonBottom,  0.0f, 0.0f,
        buttonRight, buttonTop,     1.0f, 1.0f,
        buttonLeft,  buttonTop,     0.0f, 1.0f
    };
    float pozdravVertices[] = {
        pozdravLeft,  pozdravBottom,  0.0f, 0.0f,
        pozdravRight, pozdravBottom,  1.0f, 0.0f,
        pozdravRight, pozdravTop,     1.0f, 1.0f,
        pozdravLeft,  pozdravBottom,  0.0f, 0.0f,
        pozdravRight, pozdravTop,     1.0f, 1.0f,
        pozdravLeft,  pozdravTop,     0.0f, 1.0f
    };

    // 2D VAOs
    unsigned int bgVao, credVao, buttonVao, pozdravVao;
    formVAOs(bgVertices, sizeof(bgVertices), bgVao);
    formVAOs(creditVertices, sizeof(creditVertices), credVao);
    formVAOs(buttonVertices, sizeof(buttonVertices), buttonVao);
    formVAOs(pozdravVertices, sizeof(pozdravVertices), pozdravVao);

    // Shaders
    unsigned int rectShader = createShader("rect.vert", "rect.frag");
    unsigned int shader3D = createShader("basic3d.vert", "basic3d.frag");
    unsigned int colorShader = createShader("color.vert", "color.frag");

    // Create 3D objects
    unsigned int cubeVAO = createCubeVAO();
    unsigned int planeVAO = createPlaneVAO();

    float barBgVertices[] = {
    barLeft,  barBottom,
    barRight, barBottom,
    barRight, barTop,
    barLeft,  barTop
    };

    glGenVertexArrays(1, &barBgVAO);
    glGenBuffers(1, &barBgVBO);
    glBindVertexArray(barBgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, barBgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(barBgVertices), barBgVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &barFillVAO);
    glGenBuffers(1, &barFillVBO);
    glBindVertexArray(barFillVAO);
    glBindBuffer(GL_ARRAY_BUFFER, barFillVBO);
    // 4 floats per vertex (x, y, u, v) to match your formVAOs structure even if we don't use UVs for color
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Load models
    grillModel = loadOBJModel("models/grill.obj", "models/grill.mtl");
    if (grillModel.VAO == 0) {
        std::cout << "Warning: Grill model failed to load!" << std::endl;
    }

    pattyModel = loadOBJModel("models/patty.obj", "models/patty.mtl");
    if (pattyModel.VAO == 0) {
        std::cout << "Warning: Patty model failed to load!" << std::endl;
    }

    tableModel = loadOBJModel("models/table.obj", "models/table.mtl");
    if (tableModel.VAO == 0) {
        std::cout << "Warning: Table model failed to load!" << std::endl;
    }

    plateModel = loadOBJModel("models/plate.obj", "models/plate.mtl");
    if (plateModel.VAO == 0) {
        std::cout << "Warning: Plate model failed to load!" << std::endl;
    }

    bunBottomModel = loadOBJModel("models/bun_bottom.obj", "models/bun_bottom.mtl");
    if (bunBottomModel.VAO == 0) {
        std::cout << "Warning: Bun bottom model failed to load!" << std::endl;
    }

    cookedPattyModel = loadOBJModel("models/patty.obj", "models/patty.mtl");
    if (cookedPattyModel.VAO == 0) {
        std::cout << "Warning: Cooked patty model failed to load!" << std::endl;
    }

    ketchupModel = loadOBJModel("models/ketchup.obj", "models/ketchup.mtl");
    if (ketchupModel.VAO == 0) {
        std::cout << "Warning: Ketchup model failed to load!" << std::endl;
    }

    mustardModel = loadOBJModel("models/mustard.obj", "models/mustard.mtl");
    if (mustardModel.VAO == 0) {
        std::cout << "Warning: Mustard model failed to load!" << std::endl;
    }

    picklesModel = loadOBJModel("models/pickles.obj", "models/pickles.mtl");
    if (picklesModel.VAO == 0) {
        std::cout << "Warning: pickles model failed to load!" << std::endl;
    }

    onionModel = loadOBJModel("models/onion.obj", "models/onion.mtl");
    if (onionModel.VAO == 0) {
        std::cout << "Warning: onion model failed to load!" << std::endl;
    }

    lettuceModel = loadOBJModel("models/lettuce.obj", "models/lettuce.mtl");
    if (lettuceModel.VAO == 0) {
        std::cout << "Warning: lettuce model failed to load!" << std::endl;
    }

    cheeseModel = loadOBJModel("models/cheese.obj", "models/cheese.mtl");
    if (cheeseModel.VAO == 0) {
        std::cout << "Warning: cheese model failed to load!" << std::endl;
    }

    tomatoModel = loadOBJModel("models/tomato.obj", "models/tomato.mtl");
    if (tomatoModel.VAO == 0) {
        std::cout << "Warning: tomato model failed to load!" << std::endl;
    }

    bunTopModel = loadOBJModel("models/bun_top.obj", "models/bun_top.mtl");
    if (bunTopModel.VAO == 0) {
        std::cout << "Warning: Bun top model failed to load!" << std::endl;
    }

    // Projection matrix (perspective)
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
        (float)mode->width / (float)mode->height, 0.1f, 100.0f);

    // FPS limit
    const double fpsLimit = 1.0 / 75.0;

    // Ingredient ordering and tracking
    std::map<std::string, Model> ingredients;
    ingredients["bunBottom"] = bunBottomModel;
    ingredients["cookedPatty"] = cookedPattyModel;
    ingredients["ketchup"] = ketchupModel;
    ingredients["mustard"] = mustardModel;
    ingredients["pickles"] = picklesModel;
    ingredients["onion"] = onionModel;
    ingredients["lettuce"] = lettuceModel;
    ingredients["cheese"] = cheeseModel;
    ingredients["tomato"] = tomatoModel;
    ingredients["bunTop"] = bunTopModel;

    std::vector<std::string> ingredientOrder = {
        "bunBottom", "cookedPatty", "ketchup", "mustard",
        "pickles", "onion", "lettuce", "cheese", "tomato", "bunTop"
    };
    int currentIngredientIndex = 0;
    activeIngredient = ingredientOrder[currentIngredientIndex];

    std::vector<std::string> placedIngredients;
    std::map<std::string, glm::vec3> placedIngredientPositions;

    // Height offset for stacking ingredients
    float ingredientHeightOffset = 0.15f;

    // VAOs for 2D stain rendering
    unsigned int stainVAO;
    float stainVertices[] = {
        -0.15f, -0.15f,  0.0f, 0.0f,
         0.15f, -0.15f,  1.0f, 0.0f,
         0.15f,  0.15f,  1.0f, 1.0f,
        -0.15f, -0.15f,  0.0f, 0.0f,
         0.15f,  0.15f,  1.0f, 1.0f,
        -0.15f,  0.15f,  0.0f, 1.0f
    };
    formVAOs(stainVertices, sizeof(stainVertices), stainVAO);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        double frameStart = glfwGetTime();

        if (!buttonClicked) {
            // 2D Menu mode - disable depth test for 2D rendering
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(rectShader);
            glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);

            // Draw background
            glBindTexture(GL_TEXTURE_2D, bg);
            glBindVertexArray(bgVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Draw credentials
            glBindTexture(GL_TEXTURE_2D, credentials);
            glBindVertexArray(credVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Draw button
            glBindTexture(GL_TEXTURE_2D, button);
            glBindVertexArray(buttonVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        else if (gameCompleted) {
            // Game completed - show end screen
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(rectShader);
            glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);

            // Draw background
            glBindTexture(GL_TEXTURE_2D, bg);
            glBindVertexArray(bgVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Draw prijatno (enjoy your meal)
            glBindTexture(GL_TEXTURE_2D, pozdrav);
            glBindVertexArray(pozdravVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        else {
            // 3D Game mode - enable depth test
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // View matrix (camera)
            glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

            // Use 3D shader
            glUseProgram(shader3D);

            // Set projection and view matrices
            glUniformMatrix4fv(glGetUniformLocation(shader3D, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(shader3D, "uView"), 1, GL_FALSE, glm::value_ptr(view));

            // Set lighting uniforms
            glUniform3fv(glGetUniformLocation(shader3D, "uLightPos"), 1, glm::value_ptr(lightPos));
            glUniform3fv(glGetUniformLocation(shader3D, "uViewPos"), 1, glm::value_ptr(cameraPos));
            glUniform3fv(glGetUniformLocation(shader3D, "uLightColor"), 1, glm::value_ptr(lightColor));
            glUniform1i(glGetUniformLocation(shader3D, "uLightEnabled"), lightEnabled);
            glUniform1i(glGetUniformLocation(shader3D, "uTexture"), 0);

            // Draw ground plane with floor texture
            glUniform1i(glGetUniformLocation(shader3D, "uUseMaterial"), GL_FALSE);
            glm::mat4 modelPlane = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shader3D, "uModel"), 1, GL_FALSE, glm::value_ptr(modelPlane));
            glBindTexture(GL_TEXTURE_2D, floorTex);
            glBindVertexArray(planeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);


            // Draw grill and patty during cooking phase
            if (cookedness < 1.0f) {
                glm::mat4 modelGrill = glm::mat4(1.0f);
                modelGrill = glm::translate(modelGrill, glm::vec3(-5.0f, 0.0f, 0.0f));
                modelGrill = glm::scale(modelGrill, glm::vec3(1.0f, 1.0f, 1.0f));
                renderModel(grillModel, shader3D, modelGrill, false);

                // Draw patty with cookedness-based color
                glm::mat4 modelPatty = glm::mat4(1.0f);
                modelPatty = glm::translate(modelPatty, pattyPos);
                modelPatty = glm::scale(modelPatty, glm::vec3(0.5f, 0.5f, 0.5f));
                renderModel(pattyModel, shader3D, modelPatty, true, cookedness);

                // Check if patty is on grill and cook it
                if (isPattyOnGrill()) {
                    cookedness += 0.0075f;  // Cook faster for better feedback
                    if (cookedness >= 1.0f) {
                        cookedness = 1.0f;  // Clamp to 1.0
                        finalCookedness = 1.0f; // Store final value
                    }
                }

                // --- 2. RENDERING: 2D Overlay (Loading Bar) ---
                glDisable(GL_DEPTH_TEST); // Ensure UI is on top

                // Draw Background Outline
                glUseProgram(colorShader);
                glUniform4f(glGetUniformLocation(colorShader, "uColor"), 1.0f, 1.0f, 1.0f, 1.0f);
                glBindVertexArray(barBgVAO);
                glDrawArrays(GL_LINE_LOOP, 0, 4);
                if (cookedness > 0.0f) {
                    float fillLeft = barLeft + 0.01f;
                    float fillRight = barLeft + (barRight - barLeft) * cookedness - 0.01f;
                    float fillTop = barTop - 0.01f;
                    float fillBottom = barBottom + 0.01f;

                    float barFillVertices[] = {
                        fillLeft,  fillBottom, 0.0f, 0.0f,
                        fillRight, fillBottom, 0.0f, 0.0f,
                        fillRight, fillTop,    0.0f, 0.0f,
                        fillLeft,  fillBottom, 0.0f, 0.0f,
                        fillRight, fillTop,    0.0f, 0.0f,
                        fillLeft,  fillTop,    0.0f, 0.0f
                    };

                    glBindBuffer(GL_ARRAY_BUFFER, barFillVBO);
                    glBufferData(GL_ARRAY_BUFFER, sizeof(barFillVertices), barFillVertices, GL_DYNAMIC_DRAW);

                    glUniform4f(glGetUniformLocation(colorShader, "uColor"), 0.0f, 1.0f, 0.0f, 1.0f);
                    glBindVertexArray(barFillVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
                glEnable(GL_DEPTH_TEST); // Re-enable for the 3D models
            }
            else if (cookedness >= 1.0f && cookedness < 2.0f) {
                // Transition phase - remove grill, remove patty (for now)
                cookedness = 6.7f;
            }
            else {
                // --- Draw table ---
                glm::mat4 modelTable = glm::mat4(1.0f);
                modelTable = glm::translate(modelTable, glm::vec3(-5.0f, 0.0f, 0.0f));
                modelTable = glm::scale(modelTable, glm::vec3(1.0f));
                glm::vec3 tableBrown = glm::vec3(0.35f, 0.2f, 0.1f); // darker than patty
                renderModel(tableModel, shader3D, modelTable, false, 0.0f, true, tableBrown);

                // --- Draw plate ---
                glm::mat4 modelPlate = glm::mat4(1.0f);

                if (!platePlaced)
                    modelPlate = glm::translate(modelPlate, platePos);
                else
                    modelPlate = glm::translate(modelPlate, fixedPlatePos);

                modelPlate = glm::scale(modelPlate, glm::vec3(0.02f));
                renderModel(plateModel, shader3D, modelPlate, false);

                // --- Check if plate is on table ---
                if (!platePlaced) {
                    bool withinX =
                        platePos.x >= -5.0f - 1.5f &&
                        platePos.x <= -5.0f + 1.5f;

                    bool withinZ =
                        platePos.z >= 0.0f - 1.5f &&
                        platePos.z <= 0.0f + 1.5f;

                    bool heightMatch =
                        platePos.y >= 1.7f && platePos.y <= 2.0f;

                    if (withinX && withinZ && heightMatch) {
                        platePlaced = true;
                        fixedPlatePos = platePos;
                        // Initialize first ingredient position
                        ingredientPos = fixedPlatePos + glm::vec3(0.0f, ingredientHeightOffset, 0.0f);
                    }
                }

                // --- INGREDIENT STACKING PHASE ---
                if (platePlaced) {
                    // Draw all placed ingredients
                    for (size_t i = 0; i < placedIngredients.size(); i++) {
                        const std::string& ing = placedIngredients[i];
                        glm::vec3 pos = placedIngredientPositions[ing];

                        glm::mat4 modelIng = glm::mat4(1.0f);
                        modelIng = glm::translate(modelIng, pos);

                        // Apply specific scales and rotations for different ingredients
                        if (ing == "bunBottom") {
                            // Rotate 180 degrees around X axis to face upward
                            modelIng = glm::rotate(modelIng, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                            modelIng = glm::scale(modelIng, glm::vec3(1.5f)); // 3x bigger
                        }
                        else if (ing == "bunTop") {
                            modelIng = glm::scale(modelIng, glm::vec3(1.5f)); // Same size as bunBottom
                        }
                        else if (ing == "lettuce") {
                            modelIng = glm::scale(modelIng, glm::vec3(2.67f)); // Reduced by 1.5x from 4.0
                        }
                        else if (ing == "tomato") {
                            modelIng = glm::scale(modelIng, glm::vec3(13.33f)); // Reduced by 3x from 40.0
                        }
                        else if (ing == "cheese") {
                            modelIng = glm::scale(modelIng, glm::vec3(0.01f)); // 50x smaller
                        }
                        else if (ing == "ketchup" || ing == "mustard") {
                            // Rotate 180 degrees around X axis to face floor
                            modelIng = glm::rotate(modelIng, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                            modelIng = glm::scale(modelIng, glm::vec3(0.5f));
                        }
                        else {
                            modelIng = glm::scale(modelIng, glm::vec3(0.5f));
                        }

                        glm::vec3 color = getIngredientColor(ing);

                        // Use finalCookedness for cookedPatty
                        if (ing == "cookedPatty") {
                            renderModel(ingredients[ing], shader3D, modelIng, true, finalCookedness);
                        }
                        else {
                            renderModel(ingredients[ing], shader3D, modelIng, false, 0.0f, true, color);
                        }
                    }

                    // Draw current ingredient being placed
                    if (currentIngredientIndex < ingredientOrder.size()) {
                        activeIngredient = ingredientOrder[currentIngredientIndex];

                        // Ketchup and mustard float and fire
                        if (activeIngredient == "ketchup" || activeIngredient == "mustard") {
                            glm::mat4 modelIng = glm::mat4(1.0f);
                            modelIng = glm::translate(modelIng, ingredientPos);
                            // Rotate 180 degrees around X axis to face floor
                            modelIng = glm::rotate(modelIng, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                            modelIng = glm::scale(modelIng, glm::vec3(0.5f));
                            glm::vec3 color = getIngredientColor(activeIngredient);
                            renderModel(ingredients[activeIngredient], shader3D, modelIng, false, 0.0f, true, color);

                            // Check if fired
                            if (fired) {
                                // Check if above the last placed ingredient
                                bool hit = false;
                                if (!placedIngredients.empty()) {
                                    std::string lastIng = placedIngredients.back();
                                    glm::vec3 lastPos = placedIngredientPositions[lastIng];

                                    float horizontalDist = glm::length(glm::vec2(ingredientPos.x - lastPos.x,
                                        ingredientPos.z - lastPos.z));

                                    if (horizontalDist < 0.5f) { // Hit threshold
                                        hit = true;
                                        // Place stain on top of burger
                                        Stain s;
                                        s.type = activeIngredient;
                                        s.position = lastPos + glm::vec3(0.0f, 0.3f, 0.0f);
                                        s.onPlate = true;
                                        stains.push_back(s);

                                        // Move to next ingredient only on hit
                                        currentIngredientIndex++;
                                        if (currentIngredientIndex < ingredientOrder.size()) {
                                            activeIngredient = ingredientOrder[currentIngredientIndex];
                                            ingredientPos = glm::vec3(-5.0f, 10.0f, 0.0f);
                                        }
                                    }
                                }

                                if (!hit) {
                                    // Missed - place stain on floor, but don't advance
                                    Stain s;
                                    s.type = activeIngredient;
                                    s.position = glm::vec3(ingredientPos.x, 0.01f, ingredientPos.z);
                                    s.onPlate = false;
                                    stains.push_back(s);
                                    // Don't advance - let them try again
                                }

                                // Reset fired flag regardless of hit/miss
                                fired = false;
                            }
                        }
                        else {
                            // Regular ingredient - move and place
                            glm::mat4 modelIng = glm::mat4(1.0f);
                            modelIng = glm::translate(modelIng, ingredientPos);

                            // Apply specific scales and rotations for different ingredients
                            if (activeIngredient == "bunBottom") {
                                // Rotate 180 degrees around X axis to face upward
                                modelIng = glm::rotate(modelIng, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                                modelIng = glm::scale(modelIng, glm::vec3(1.5f)); // 3x bigger
                            }
                            else if (activeIngredient == "bunTop") {
                                modelIng = glm::scale(modelIng, glm::vec3(1.5f)); // Same size as bunBottom
                            }
                            else if (activeIngredient == "lettuce") {
                                modelIng = glm::scale(modelIng, glm::vec3(2.67f)); // Reduced by 1.5x from 4.0
                            }
                            else if (activeIngredient == "tomato") {
                                modelIng = glm::scale(modelIng, glm::vec3(13.33f)); // Reduced by 3x from 40.0
                            }
                            else if (activeIngredient == "cheese") {
                                modelIng = glm::scale(modelIng, glm::vec3(0.01f)); // 50x smaller
                            }
                            else {
                                modelIng = glm::scale(modelIng, glm::vec3(0.5f));
                            }

                            glm::vec3 color = getIngredientColor(activeIngredient);

                            if (activeIngredient == "cookedPatty") {
                                renderModel(ingredients[activeIngredient], shader3D, modelIng, true, finalCookedness);
                            }
                            else {
                                renderModel(ingredients[activeIngredient], shader3D, modelIng, false, 0.0f, true, color);
                            }

                            // Check if placed on top of previous ingredient (or plate if first)
                            glm::vec3 targetPos;
                            if (placedIngredients.empty()) {
                                targetPos = fixedPlatePos;
                            }
                            else {
                                targetPos = placedIngredientPositions[placedIngredients.back()];
                            }

                            float horizontalDist = glm::length(glm::vec2(ingredientPos.x - targetPos.x,
                                ingredientPos.z - targetPos.z));
                            float verticalDist = std::abs(ingredientPos.y - (targetPos.y + ingredientHeightOffset));

                            if (horizontalDist < 0.3f && verticalDist < 0.2f) {
                                // Placed successfully
                                placedIngredients.push_back(activeIngredient);
                                placedIngredientPositions[activeIngredient] = ingredientPos;

                                // Check if this was the last ingredient (bunTop)
                                if (activeIngredient == "bunTop") {
                                    gameCompleted = true;
                                    // Re-enable cursor
                                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                                }

                                // Move to next ingredient
                                currentIngredientIndex++;
                                if (currentIngredientIndex < ingredientOrder.size()) {
                                    activeIngredient = ingredientOrder[currentIngredientIndex];
                                    ingredientPos = glm::vec3(-5.0f, 10.0f, 0.0f);
                                }
                            }
                        }
                    }

                    // Draw stains
                    glDisable(GL_DEPTH_TEST);
                    glUseProgram(rectShader);
                    glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);

                    for (const Stain& s : stains) {
                        // Convert 3D position to screen space for 2D rendering
                        glm::vec4 clipPos = projection * view * glm::vec4(s.position, 1.0f);
                        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

                        if (clipPos.w > 0 && ndc.z > -1.0f && ndc.z < 1.0f) {
                            // Create transformation for stain
                            float stainSize = 0.1f;
                            float stainVertices2D[] = {
                                ndc.x - stainSize, ndc.y - stainSize,  0.0f, 0.0f,
                                ndc.x + stainSize, ndc.y - stainSize,  1.0f, 0.0f,
                                ndc.x + stainSize, ndc.y + stainSize,  1.0f, 1.0f,
                                ndc.x - stainSize, ndc.y - stainSize,  0.0f, 0.0f,
                                ndc.x + stainSize, ndc.y + stainSize,  1.0f, 1.0f,
                                ndc.x - stainSize, ndc.y + stainSize,  0.0f, 1.0f
                            };

                            unsigned int tempVAO;
                            formVAOs(stainVertices2D, sizeof(stainVertices2D), tempVAO);

                            if (s.type == "ketchup") {
                                glBindTexture(GL_TEXTURE_2D, ketchupStain);
                            }
                            else {
                                glBindTexture(GL_TEXTURE_2D, mustardStain);
                            }

                            glBindVertexArray(tempVAO);
                            glDrawArrays(GL_TRIANGLES, 0, 6);
                        }
                    }

                    glEnable(GL_DEPTH_TEST);
                }
            }

            // Draw 2D credentials overlay
            glDisable(GL_DEPTH_TEST);
            glUseProgram(rectShader);
            glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);
            glBindTexture(GL_TEXTURE_2D, credentials);
            glBindVertexArray(credVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);
        } // End of 3D game mode else block

        glfwSwapBuffers(window);
        glfwPollEvents();

        // FPS limit
        double frameEnd = glfwGetTime();
        double delta = frameEnd - frameStart;
        if (delta < fpsLimit) {
            while (glfwGetTime() < frameStart + fpsLimit) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    glfwTerminate();
    return 0;
}