//SV20/2022

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
#include <map>

//=== Camera Variables ===
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, -10.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.3f;
float yaw = 90.0f;
float pitch = -20.0f;
bool firstMouse = true;
double lastX = 400.0, lastY = 300.0;

//=== Light Variables ===
glm::vec3 lightPos = glm::vec3(0.0f, 10.0f, 0.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
bool lightEnabled = true;

//=== Patty Variables ===
glm::vec3 pattyPos = glm::vec3(-5.0f, 5.0f, 0.0f);
float pattyMoveSpeed = 0.1f;
float cookedness = 0.0f;
float finalCookedness = 0.0f;

//=== Plate Variables ===
glm::vec3 platePos = glm::vec3(-5.0f, 5.0f, 0.0f);
float plateMoveSpeed = 0.1f;
bool platePlaced = false;
glm::vec3 fixedPlatePos;

//=== Ingredient Variables ===
glm::vec3 ingredientPos = glm::vec3(-5.0f, 10.0f, 0.0f);
float ingredientSpeed = 0.1f;
bool ingredientPlaced = false;
glm::vec3 fixedIngredientPos;
std::string activeIngredient = "bunBottom";
float ingredientHeightOffset = 0.15f;

//=== Game State ===
bool buttonClicked = false;
bool fired = false;
bool gameCompleted = false;

//=== Grill Configuration ===
glm::vec3 grillPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec2 grillHalfSize = glm::vec2(1.5f, 1.5f);

//=== UI Elements ===
unsigned bg, floorTex, credentials, button, pozdrav;
unsigned ketchupStain, mustardStain;
GLFWcursor* cursor;

//=== UI Bounds ===
float buttonLeft = -0.2f, buttonRight = 0.2f;
float buttonTop = 0.2f, buttonBottom = -0.2f;
float pozdravLeft = -0.3f, pozdravRight = 0.3f;
float pozdravTop = 0.8f, pozdravBottom = 0.5f;
float barLeft = -0.95f, barRight = -0.55f;
float barTop = 0.95f, barBottom = 0.85f;

//=== VAO/VBO for UI ===
unsigned barBgVAO, barBgVBO;
unsigned barFillVAO, barFillVBO;

//=== 3D Models ===
Model grillModel, pattyModel, tableModel, plateModel;
Model bunBottomModel, cookedPattyModel, ketchupModel, mustardModel;
Model picklesModel, onionModel, lettuceModel, cheeseModel, tomatoModel, bunTopModel;

//=== Stain Tracking ===
struct Stain {
    std::string type; //"ketchup" or "mustard"
    glm::vec3 position;
    bool onPlate; //true if on plate/burger, false if on floor
};
std::vector<Stain> stains;

int endProgram(const char* msg) {
    std::cout << msg << std::endl;
    glfwTerminate();
    return -1;
}

void mouse_motion_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!buttonClicked) return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    //constrain pitch
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    //update camera front vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!buttonClicked) return;

    //camera movement (arrow keys)
    if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos += cameraSpeed * cameraFront;
    if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos -= cameraSpeed * cameraFront;
    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    //light toggle
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
        lightEnabled = !lightEnabled;

    //patty movement (WASD for X/Z, Shift/Space for Y) - only when cooking
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
    //plate movement - after cooking, before placing
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
    //ingredient movement - after plate is placed
    else if (platePlaced) {
        if (activeIngredient == "ketchup" || activeIngredient == "mustard") {
            //ketchup/mustard only move in X/Z, fire with space
            if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.x += ingredientSpeed;
            if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.x -= ingredientSpeed;
            if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.z += ingredientSpeed;
            if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
                ingredientPos.z -= ingredientSpeed;
            if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
                fired = true;
        }
        else {
            //regular ingredients move in all directions
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
    if (!buttonClicked && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        double x_ndc = (xpos / width) * 2.0 - 1.0;
        double y_ndc = 1.0 - (ypos / height) * 2.0;

        if (x_ndc >= buttonLeft && x_ndc <= buttonRight &&
            y_ndc >= buttonBottom && y_ndc <= buttonTop) {
            buttonClicked = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

bool isPattyOnGrill() {
    bool withinX = pattyPos.x >= grillPosition.x - grillHalfSize.x &&
        pattyPos.x <= grillPosition.x + grillHalfSize.x;
    bool withinZ = pattyPos.z >= grillPosition.z - grillHalfSize.y &&
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

unsigned int createCubeVAO() {
    float vertices[] = {
        //positions          //normals           //tex coords
        //back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        //front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        //left face
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        //right face
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         //bottom face
         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
          0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
          0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
          0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         //top face
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return VAO;
}

unsigned int createPlaneVAO() {
    float vertices[] = {
        //positions          //normals        //tex coords
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

void renderModel(const Model& model, unsigned int shader, const glm::mat4& modelMatrix,
    bool isPatty = false, float cookedness = 0.0f,
    bool forceColor = false, glm::vec3 forcedColor = glm::vec3(1.0f)) {
    glUniformMatrix4fv(glGetUniformLocation(shader, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

    if (isPatty) {
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

    //reset uniforms
    glUniform1i(glGetUniformLocation(shader, "uUseCooked"), GL_FALSE);
    glUniform1i(glGetUniformLocation(shader, "uUseMaterial"), GL_FALSE);
}

glm::vec3 getIngredientColor(const std::string& ingredient) {
    if (ingredient == "bunBottom" || ingredient == "bunTop")
        return glm::vec3(0.85f, 0.7f, 0.45f);
    else if (ingredient == "ketchup")
        return glm::vec3(0.8f, 0.1f, 0.1f);
    else if (ingredient == "mustard")
        return glm::vec3(0.95f, 0.85f, 0.1f);
    else if (ingredient == "pickles")
        return glm::vec3(0.3f, 0.6f, 0.2f);
    else if (ingredient == "onion")
        return glm::vec3(0.95f, 0.9f, 0.85f);
    else if (ingredient == "lettuce")
        return glm::vec3(0.4f, 0.75f, 0.3f);
    else if (ingredient == "cheese")
        return glm::vec3(1.0f, 0.8f, 0.2f);
    else if (ingredient == "tomato")
        return glm::vec3(0.9f, 0.2f, 0.2f);

    return glm::vec3(0.5f, 0.5f, 0.5f);
}

int main() {
    //=== GLFW Initialization ===
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //=== Load Textures ===
    preprocessTexture(bg, "res/background.png");
    preprocessTexture(floorTex, "res/floor.png");
    preprocessTexture(credentials, "res/credentials.png");
    preprocessTexture(button, "res/button.png");
    preprocessTexture(ketchupStain, "res/ketchup_stain.png");
    preprocessTexture(mustardStain, "res/mustard_stain.png");
    preprocessTexture(pozdrav, "res/prijatno.png");

    //=== Create 2D Menu Vertices ===
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
         0.5f,  0.6f,  0.0f, 0.0f,
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

    //=== Create 2D VAOs ===
    unsigned int bgVao, credVao, buttonVao, pozdravVao;
    formVAOs(bgVertices, sizeof(bgVertices), bgVao);
    formVAOs(creditVertices, sizeof(creditVertices), credVao);
    formVAOs(buttonVertices, sizeof(buttonVertices), buttonVao);
    formVAOs(pozdravVertices, sizeof(pozdravVertices), pozdravVao);

    //=== Load Shaders ===
    unsigned int rectShader = createShader("rect.vert", "rect.frag");
    unsigned int shader3D = createShader("basic3d.vert", "basic3d.frag");
    unsigned int colorShader = createShader("color.vert", "color.frag");

    //=== Create 3D Primitives ===
    unsigned int cubeVAO = createCubeVAO();
    unsigned int planeVAO = createPlaneVAO();

    //=== Create Progress Bar VAOs ===
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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //=== Load 3D Models ===
    grillModel = loadOBJModel("models/grill.obj", "models/grill.mtl");
    pattyModel = loadOBJModel("models/patty.obj", "models/patty.mtl");
    tableModel = loadOBJModel("models/table.obj", "models/table.mtl");
    plateModel = loadOBJModel("models/plate.obj", "models/plate.mtl");
    bunBottomModel = loadOBJModel("models/bun_bottom.obj", "models/bun_bottom.mtl");
    cookedPattyModel = loadOBJModel("models/patty.obj", "models/patty.mtl");
    ketchupModel = loadOBJModel("models/ketchup.obj", "models/ketchup.mtl");
    mustardModel = loadOBJModel("models/mustard.obj", "models/mustard.mtl");
    picklesModel = loadOBJModel("models/pickles.obj", "models/pickles.mtl");
    onionModel = loadOBJModel("models/onion.obj", "models/onion.mtl");
    lettuceModel = loadOBJModel("models/lettuce.obj", "models/lettuce.mtl");
    cheeseModel = loadOBJModel("models/cheese.obj", "models/cheese.mtl");
    tomatoModel = loadOBJModel("models/tomato.obj", "models/tomato.mtl");
    bunTopModel = loadOBJModel("models/bun_top.obj", "models/bun_top.mtl");

    //=== Projection Matrix ===
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
        (float)mode->width / (float)mode->height, 0.1f, 100.0f);

    //=== Ingredient Order Setup ===
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

    //=== FPS Limit ===
    const double fpsLimit = 1.0 / 75.0;

    //=== Main Loop ===
    while (!glfwWindowShouldClose(window)) {
        double frameStart = glfwGetTime();

        if (!buttonClicked) {
            //2D menu mode
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(rectShader);
            glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);

            glBindTexture(GL_TEXTURE_2D, bg);
            glBindVertexArray(bgVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBindTexture(GL_TEXTURE_2D, credentials);
            glBindVertexArray(credVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBindTexture(GL_TEXTURE_2D, button);
            glBindVertexArray(buttonVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        else if (gameCompleted) {
            //game completion screen
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(rectShader);
            glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);

            glBindTexture(GL_TEXTURE_2D, bg);
            glBindVertexArray(bgVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBindTexture(GL_TEXTURE_2D, pozdrav);
            glBindVertexArray(pozdravVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        else {
            //3D game mode
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

            glUseProgram(shader3D);
            glUniformMatrix4fv(glGetUniformLocation(shader3D, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(shader3D, "uView"), 1, GL_FALSE, glm::value_ptr(view));
            glUniform3fv(glGetUniformLocation(shader3D, "uLightPos"), 1, glm::value_ptr(lightPos));
            glUniform3fv(glGetUniformLocation(shader3D, "uViewPos"), 1, glm::value_ptr(cameraPos));
            glUniform3fv(glGetUniformLocation(shader3D, "uLightColor"), 1, glm::value_ptr(lightColor));
            glUniform1i(glGetUniformLocation(shader3D, "uLightEnabled"), lightEnabled);
            glUniform1i(glGetUniformLocation(shader3D, "uTexture"), 0);

            //draw ground plane
            glUniform1i(glGetUniformLocation(shader3D, "uUseMaterial"), GL_FALSE);
            glm::mat4 modelPlane = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shader3D, "uModel"), 1, GL_FALSE, glm::value_ptr(modelPlane));
            glBindTexture(GL_TEXTURE_2D, floorTex);
            glBindVertexArray(planeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            if (cookedness < 1.0f) {
                //=== COOKING PHASE ===
                //draw grill
                glm::mat4 modelGrill = glm::mat4(1.0f);
                modelGrill = glm::translate(modelGrill, glm::vec3(-5.0f, 0.0f, 0.0f));
                renderModel(grillModel, shader3D, modelGrill);

                //draw patty
                glm::mat4 modelPatty = glm::mat4(1.0f);
                modelPatty = glm::translate(modelPatty, pattyPos);
                modelPatty = glm::scale(modelPatty, glm::vec3(0.5f));
                renderModel(pattyModel, shader3D, modelPatty, true, cookedness);

                //cook patty if on grill
                if (isPattyOnGrill()) {
                    cookedness += 0.0075f;
                    if (cookedness >= 1.0f) {
                        cookedness = 1.0f;
                        finalCookedness = 1.0f;
                    }
                }

                //draw progress bar
                glDisable(GL_DEPTH_TEST);
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
                glEnable(GL_DEPTH_TEST);
            }
            else if (cookedness >= 1.0f && cookedness < 2.0f) {
                //transition phase
                cookedness = 6.7f;
            }
            else {
                //=== PLATING PHASE ===
                //draw table
                glm::mat4 modelTable = glm::mat4(1.0f);
                modelTable = glm::translate(modelTable, glm::vec3(-5.0f, 0.0f, 0.0f));
                glm::vec3 tableBrown = glm::vec3(0.35f, 0.2f, 0.1f);
                renderModel(tableModel, shader3D, modelTable, false, 0.0f, true, tableBrown);

                //draw plate
                glm::mat4 modelPlate = glm::mat4(1.0f);
                modelPlate = glm::translate(modelPlate, platePlaced ? fixedPlatePos : platePos);
                modelPlate = glm::scale(modelPlate, glm::vec3(0.02f));
                renderModel(plateModel, shader3D, modelPlate);

                //check if plate placed on table
                if (!platePlaced) {
                    bool withinX = platePos.x >= -6.5f && platePos.x <= -3.5f;
                    bool withinZ = platePos.z >= -1.5f && platePos.z <= 1.5f;
                    bool heightMatch = platePos.y >= 1.7f && platePos.y <= 2.0f;

                    if (withinX && withinZ && heightMatch) {
                        platePlaced = true;
                        fixedPlatePos = platePos;
                        ingredientPos = fixedPlatePos + glm::vec3(0.0f, ingredientHeightOffset, 0.0f);
                    }
                }

                //=== INGREDIENT STACKING PHASE ===
                if (platePlaced) {
                    //draw all placed ingredients
                    for (const std::string& ing : placedIngredients) {
                        glm::vec3 pos = placedIngredientPositions[ing];
                        glm::mat4 modelIng = glm::mat4(1.0f);
                        modelIng = glm::translate(modelIng, pos);

                        //apply ingredient-specific transforms
                        if (ing == "bunBottom") {
                            modelIng = glm::rotate(modelIng, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                            modelIng = glm::scale(modelIng, glm::vec3(1.5f));
                        }
                        else if (ing == "bunTop") {
                            modelIng = glm::scale(modelIng, glm::vec3(1.5f));
                        }
                        else if (ing == "lettuce") {
                            modelIng = glm::scale(modelIng, glm::vec3(2.67f));
                        }
                        else if (ing == "tomato") {
                            modelIng = glm::scale(modelIng, glm::vec3(13.33f));
                        }
                        else if (ing == "cheese") {
                            modelIng = glm::scale(modelIng, glm::vec3(0.01f));
                        }
                        else if (ing == "ketchup" || ing == "mustard") {
                            modelIng = glm::rotate(modelIng, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                            modelIng = glm::scale(modelIng, glm::vec3(0.5f));
                        }
                        else {
                            modelIng = glm::scale(modelIng, glm::vec3(0.5f));
                        }

                        if (ing == "cookedPatty") {
                            renderModel(ingredients[ing], shader3D, modelIng, true, finalCookedness);
                        }
                        else {
                            renderModel(ingredients[ing], shader3D, modelIng, false, 0.0f, true, getIngredientColor(ing));
                        }
                    }

                    //draw current ingredient being placed
                    if (currentIngredientIndex < ingredientOrder.size()) {
                        activeIngredient = ingredientOrder[currentIngredientIndex];

                        if (activeIngredient == "ketchup" || activeIngredient == "mustard") {
                            //draw floating sauce bottle
                            glm::mat4 modelIng = glm::mat4(1.0f);
                            modelIng = glm::translate(modelIng, ingredientPos);
                            modelIng = glm::rotate(modelIng, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                            modelIng = glm::scale(modelIng, glm::vec3(0.5f));
                            renderModel(ingredients[activeIngredient], shader3D, modelIng, false, 0.0f, true, getIngredientColor(activeIngredient));

                            //check if fired
                            if (fired) {
                                bool hit = false;
                                if (!placedIngredients.empty()) {
                                    glm::vec3 lastPos = placedIngredientPositions[placedIngredients.back()];
                                    float horizontalDist = glm::length(glm::vec2(ingredientPos.x - lastPos.x, ingredientPos.z - lastPos.z));

                                    if (horizontalDist < 0.5f) {
                                        hit = true;
                                        //place stain on burger
                                        Stain s;
                                        s.type = activeIngredient;
                                        s.position = lastPos + glm::vec3(0.0f, 0.3f, 0.0f);
                                        s.onPlate = true;
                                        stains.push_back(s);

                                        //advance to next ingredient
                                        currentIngredientIndex++;
                                        if (currentIngredientIndex < ingredientOrder.size()) {
                                            ingredientPos = glm::vec3(-5.0f, 10.0f, 0.0f);
                                        }
                                    }
                                }

                                if (!hit) {
                                    //missed - place stain on floor
                                    Stain s;
                                    s.type = activeIngredient;
                                    s.position = glm::vec3(ingredientPos.x, 0.01f, ingredientPos.z);
                                    s.onPlate = false;
                                    stains.push_back(s);
                                }

                                fired = false;
                            }
                        }
                        else {
                            //draw regular ingredient
                            glm::mat4 modelIng = glm::mat4(1.0f);
                            modelIng = glm::translate(modelIng, ingredientPos);

                            if (activeIngredient == "bunBottom") {
                                modelIng = glm::rotate(modelIng, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                                modelIng = glm::scale(modelIng, glm::vec3(1.5f));
                            }
                            else if (activeIngredient == "bunTop") {
                                modelIng = glm::scale(modelIng, glm::vec3(1.5f));
                            }
                            else if (activeIngredient == "lettuce") {
                                modelIng = glm::scale(modelIng, glm::vec3(2.67f));
                            }
                            else if (activeIngredient == "tomato") {
                                modelIng = glm::scale(modelIng, glm::vec3(13.33f));
                            }
                            else if (activeIngredient == "cheese") {
                                modelIng = glm::scale(modelIng, glm::vec3(0.01f));
                            }
                            else {
                                modelIng = glm::scale(modelIng, glm::vec3(0.5f));
                            }

                            if (activeIngredient == "cookedPatty") {
                                renderModel(ingredients[activeIngredient], shader3D, modelIng, true, finalCookedness);
                            }
                            else {
                                renderModel(ingredients[activeIngredient], shader3D, modelIng, false, 0.0f, true, getIngredientColor(activeIngredient));
                            }

                            //check placement
                            glm::vec3 targetPos = placedIngredients.empty() ? fixedPlatePos : placedIngredientPositions[placedIngredients.back()];
                            float horizontalDist = glm::length(glm::vec2(ingredientPos.x - targetPos.x, ingredientPos.z - targetPos.z));
                            float verticalDist = std::abs(ingredientPos.y - (targetPos.y + ingredientHeightOffset));

                            if (horizontalDist < 0.3f && verticalDist < 0.2f) {
                                placedIngredients.push_back(activeIngredient);
                                placedIngredientPositions[activeIngredient] = ingredientPos;

                                if (activeIngredient == "bunTop") {
                                    gameCompleted = true;
                                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                                }

                                currentIngredientIndex++;
                                if (currentIngredientIndex < ingredientOrder.size()) {
                                    ingredientPos = glm::vec3(-5.0f, 10.0f, 0.0f);
                                }
                            }
                        }
                    }

                    //draw stains
                    glDisable(GL_DEPTH_TEST);
                    glUseProgram(rectShader);
                    glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);

                    for (const Stain& s : stains) {
                        glm::vec4 clipPos = projection * view * glm::vec4(s.position, 1.0f);
                        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

                        if (clipPos.w > 0 && ndc.z > -1.0f && ndc.z < 1.0f) {
                            float stainSize = 0.1f;
                            float stainVertices[] = {
                                ndc.x - stainSize, ndc.y - stainSize,  0.0f, 0.0f,
                                ndc.x + stainSize, ndc.y - stainSize,  1.0f, 0.0f,
                                ndc.x + stainSize, ndc.y + stainSize,  1.0f, 1.0f,
                                ndc.x - stainSize, ndc.y - stainSize,  0.0f, 0.0f,
                                ndc.x + stainSize, ndc.y + stainSize,  1.0f, 1.0f,
                                ndc.x - stainSize, ndc.y + stainSize,  0.0f, 1.0f
                            };

                            unsigned int tempVAO;
                            formVAOs(stainVertices, sizeof(stainVertices), tempVAO);
                            glBindTexture(GL_TEXTURE_2D, s.type == "ketchup" ? ketchupStain : mustardStain);
                            glBindVertexArray(tempVAO);
                            glDrawArrays(GL_TRIANGLES, 0, 6);
                        }
                    }

                    glEnable(GL_DEPTH_TEST);
                }
            }

            //draw credentials overlay
            glDisable(GL_DEPTH_TEST);
            glUseProgram(rectShader);
            glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);
            glBindTexture(GL_TEXTURE_2D, credentials);
            glBindVertexArray(credVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        //FPS limiting
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