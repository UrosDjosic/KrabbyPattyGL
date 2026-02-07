#define _CRT_SECURE_NO_WARNINGS

#include "../Header/Util.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <cstring>
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../Header/stb_image.h"

// Autor: Nedeljko Tesanovic
// Opis: pomocne funkcije za zaustavljanje programa, ucitavanje sejdera, tekstura i kursora
// Smeju se koristiti tokom izrade projekta

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

unsigned int compileShader(GLenum type, const char* source)
{
    //Uzima kod u fajlu na putanji "source", kompajlira ga i vraca sejder tipa "type"
    //Citanje izvornog koda iz fajla
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
    std::string temp = ss.str();
    const char* sourceCode = temp.c_str(); //Izvorni kod sejdera koji citamo iz fajla na putanji "source"

    int shader = glCreateShader(type); //Napravimo prazan sejder odredjenog tipa (vertex ili fragment)

    int success; //Da li je kompajliranje bilo uspjesno (1 - da)
    char infoLog[512]; //Poruka o gresci (Objasnjava sta je puklo unutar sejdera)
    glShaderSource(shader, 1, &sourceCode, NULL); //Postavi izvorni kod sejdera
    glCompileShader(shader); //Kompajliraj sejder

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); //Provjeri da li je sejder uspjesno kompajliran
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog); //Pribavi poruku o gresci
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    //Pravi objedinjeni sejder program koji se sastoji od Vertex sejdera ciji je kod na putanji vsSource

    unsigned int program; //Objedinjeni sejder
    unsigned int vertexShader; //Verteks sejder (za prostorne podatke)
    unsigned int fragmentShader; //Fragment sejder (za boje, teksture itd)

    program = glCreateProgram(); //Napravi prazan objedinjeni sejder program

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource); //Napravi i kompajliraj vertex sejder
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource); //Napravi i kompajliraj fragment sejder

    //Zakaci verteks i fragment sejdere za objedinjeni program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //Povezi ih u jedan objedinjeni sejder program
    glValidateProgram(program); //Izvrsi provjeru novopecenog programa

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success); //Slicno kao za sejdere
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    //Posto su kodovi sejdera u objedinjenom sejderu, oni pojedinacni programi nam ne trebaju, pa ih brisemo zarad ustede na memoriji
    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        //Slike se osnovno ucitavaju naopako pa se moraju ispraviti da budu uspravne
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        // Provjerava koji je format boja ucitane slike
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        // oslobadjanje memorije zauzete sa stbi_load posto vise nije potrebna
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}

GLFWcursor* loadImageToCursor(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;

    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);

    if (ImageData != NULL)
    {
        GLFWimage image;
        image.width = TextureWidth;
        image.height = TextureHeight;
        image.pixels = ImageData;

        // Tacka na površini slike kursora koja se ponaša kao hitboks, moze se menjati po potrebi
        // Trenutno je gornji levi ugao, odnosno na 20% visine i 20% sirine slike kursora
        int hotspotX = TextureWidth / 5;
        int hotspotY = TextureHeight / 5;

        GLFWcursor* cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
        stbi_image_free(ImageData);
        return cursor;
    }
    else {
        std::cout << "Kursor nije ucitan! Putanja kursora: " << filePath << std::endl;
        stbi_image_free(ImageData);

    }
}

std::map<std::string, Material> loadMTL(const char* mtlPath) {
    std::map<std::string, Material> materials;
    std::ifstream file(mtlPath);

    if (!file.is_open()) {
        std::cout << "Failed to open MTL file: " << mtlPath << std::endl;
        return materials;
    }

    std::string line;
    std::string currentMaterialName;
    Material currentMaterial{};

    // Default values
    currentMaterial.ambient = glm::vec3(1.0f);
    currentMaterial.diffuse = glm::vec3(0.8f);
    currentMaterial.specular = glm::vec3(0.5f);
    currentMaterial.shininess = 32.0f;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "newmtl") {
            // Save previous material if exists
            if (!currentMaterialName.empty()) {
                materials[currentMaterialName] = currentMaterial;
            }
            // Start new material
            iss >> currentMaterialName;
            // Reset to defaults
            currentMaterial.ambient = glm::vec3(1.0f);
            currentMaterial.diffuse = glm::vec3(0.8f);
            currentMaterial.specular = glm::vec3(0.5f);
            currentMaterial.shininess = 32.0f;
        }
        else if (prefix == "Ka") {
            iss >> currentMaterial.ambient.r >> currentMaterial.ambient.g >> currentMaterial.ambient.b;
        }
        else if (prefix == "Kd") {
            iss >> currentMaterial.diffuse.r >> currentMaterial.diffuse.g >> currentMaterial.diffuse.b;
        }
        else if (prefix == "Ks") {
            iss >> currentMaterial.specular.r >> currentMaterial.specular.g >> currentMaterial.specular.b;
        }
        else if (prefix == "Ns") {
            iss >> currentMaterial.shininess;
        }
    }

    // Save last material
    if (!currentMaterialName.empty()) {
        materials[currentMaterialName] = currentMaterial;
    }

    file.close();
    std::cout << "Loaded " << materials.size() << " materials from: " << mtlPath << std::endl;
    return materials;
}

Model loadOBJModel(const char* objPath, const char* mtlPath) {
    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec3> temp_normals;
    std::vector<glm::vec2> temp_uvs;
    std::vector<Vertex> vertices;

    Model model;
    model.VAO = 0;
    model.vertexCount = 0;
    model.currentMaterial = "";

    // Load MTL file if provided
    if (mtlPath != nullptr) {
        model.materials = loadMTL(mtlPath);
    }

    std::ifstream file(objPath);
    if (!file.is_open()) {
        std::cout << "Failed to open OBJ file: " << objPath << std::endl;
        return model;
    }

    std::string line;
    std::string currentMaterial = "";

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            temp_positions.push_back(pos);
        }
        else if (prefix == "vt") {
            glm::vec2 uv;
            iss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (prefix == "usemtl") {
            iss >> currentMaterial;
            model.currentMaterial = currentMaterial;
        }
        else if (prefix == "f") {
            std::string vertex1, vertex2, vertex3;
            iss >> vertex1 >> vertex2 >> vertex3;

            auto parseFaceVertex = [&](const std::string& vertexStr) {
                Vertex vertex{};
                int posIdx = 0, uvIdx = 0, normIdx = 0;

                // Handle different face formats: v, v/vt, v/vt/vn, v//vn
                if (sscanf(vertexStr.c_str(), "%d/%d/%d", &posIdx, &uvIdx, &normIdx) == 3) {
                    // v/vt/vn
                }
                else if (sscanf(vertexStr.c_str(), "%d//%d", &posIdx, &normIdx) == 2) {
                    // v//vn
                    uvIdx = 0;
                }
                else if (sscanf(vertexStr.c_str(), "%d/%d", &posIdx, &uvIdx) == 2) {
                    // v/vt
                    normIdx = 0;
                }
                else if (sscanf(vertexStr.c_str(), "%d", &posIdx) == 1) {
                    // v
                    uvIdx = 0;
                    normIdx = 0;
                }

                if (posIdx > 0 && posIdx <= temp_positions.size()) {
                    vertex.x = temp_positions[posIdx - 1].x;
                    vertex.y = temp_positions[posIdx - 1].y;
                    vertex.z = temp_positions[posIdx - 1].z;
                }

                if (uvIdx > 0 && uvIdx <= temp_uvs.size()) {
                    vertex.u = temp_uvs[uvIdx - 1].x;
                    vertex.v = temp_uvs[uvIdx - 1].y;
                }
                else {
                    vertex.u = 0.0f;
                    vertex.v = 0.0f;
                }

                if (normIdx > 0 && normIdx <= temp_normals.size()) {
                    vertex.nx = temp_normals[normIdx - 1].x;
                    vertex.ny = temp_normals[normIdx - 1].y;
                    vertex.nz = temp_normals[normIdx - 1].z;
                }
                else {
                    // Default normal pointing up
                    vertex.nx = 0.0f;
                    vertex.ny = 1.0f;
                    vertex.nz = 0.0f;
                }

                return vertex;
                };

            vertices.push_back(parseFaceVertex(vertex1));
            vertices.push_back(parseFaceVertex(vertex2));
            vertices.push_back(parseFaceVertex(vertex3));
        }
    }
    file.close();

    if (vertices.empty()) {
        std::cout << "No vertices loaded from OBJ file!" << std::endl;
        return model;
    }

    // Create VAO/VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    std::cout << "Loaded OBJ model: " << objPath << " with " << vertices.size() << " vertices" << std::endl;

    model.VAO = VAO;
    model.vertexCount = vertices.size();

    return model;
}