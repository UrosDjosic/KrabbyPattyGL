#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>

struct Vertex {
    float x, y, z;       // Position
    float nx, ny, nz;    // Normal
    float u, v;          // Texture coordinates
};

struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

struct Model {
    unsigned int VAO;
    unsigned int vertexCount;
    std::map<std::string, Material> materials;
    std::string currentMaterial;
};

int endProgram(std::string message);
unsigned int createShader(const char* vsSource, const char* fsSource);
unsigned loadImageToTexture(const char* filePath);
GLFWcursor* loadImageToCursor(const char* filePath);
Model loadOBJModel(const char* objPath, const char* mtlPath = nullptr);