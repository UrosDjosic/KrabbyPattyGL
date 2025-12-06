//Main.cpp
//SV20/2022

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

struct Rect { float left, right, top, bottom; };
struct IngredientInfo { unsigned texture; unsigned vao; float height; };
struct StickyOverlay {
    unsigned tex;
    unsigned vao;
    float hShift;
    float vShift;
};
std::vector<StickyOverlay> stainsAndSuccesses;
struct Placed { //postavljeni sastojci
    unsigned tex;
    unsigned vao;
    float hShift;
    float vShift;
    Rect rect; //izracunato tokom izvrsavanja
};



GLFWcursor* cursor;

unsigned bg;

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

unsigned grill;
float grillLeft = -0.5f;
float grillRight = 0.5f;
float grillTop = -0.2f;
float grillBottom = -1.1f;

unsigned patty;

unsigned barBg;
unsigned barFill;
float barLeft = -0.9f;
float barRight = -0.5f;
float barTop = 0.7f;
float barBottom = 0.6f;

unsigned table;
float tableLeft = -0.5f;
float tableRight = 0.5f;
float tableTop = -0.2f;
float tableBottom = -1.1f;

unsigned plate;
float plateLeft = -0.25f;
float plateRight = 0.25f;
float plateTop = -0.35f;
float plateBottom = -0.55f;

unsigned lowerBun;
unsigned burgir;
unsigned pickles;
unsigned onion;
unsigned lettuce;
unsigned cheese;
unsigned tomato;
unsigned upperBun;

/*
iako se stekuju kao sastojci, logika za kecap i senf je kompleksnija
*/
unsigned ketchup;
unsigned ketchupSuccess;
unsigned ketchupStain;
unsigned mustard;
unsigned mustardSuccess;
unsigned mustardStain;

float ingredientLeft = -0.1f;
float ingredientRight = 0.1f;
float ingredientTop = 0.95f;
float ingredientBottom = ingredientTop - 0.1f;

float bottleLeft = -0.05f;
float bottleRight = 0.05f;
float bottleTop = 0.95f;
float bottleBottom = bottleTop - 0.25f;

float cookedness = 0.0f; //koristimo da kontrolisemo flow, prikazujemo progres, i menjamo boju

float shiftHorizontal = 0.0f; //koristimo za bilo koji trenutno aktivni sastojak
float shiftVertical = 0.0f; //koristimo za bilo koji trenutno aktivni sastojak

bool fired = false;
bool ketchupDone = false;
bool mustardDone = false;

int endProgram(const char* msg) {
    std::cout << msg << std::endl;
    glfwTerminate();
    return -1;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT)) shiftHorizontal -= 0.03f;
    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT)) shiftHorizontal += 0.03f;
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT)) shiftVertical += 0.03f;
    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) shiftVertical -= 0.03f;
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) fired = true;
}

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
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
        }
    }
}

bool objectsTouching(const Rect& a, const Rect& b)
{
    return (a.left < b.right &&
        a.right > b.left &&
        a.bottom < b.top &&
        a.top > b.bottom);
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

int main()
{
    //glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //fullscreen
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) return endProgram("Glavni monitor nije pronadjen.");

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) return endProgram("Video mod nije pronadjen.");

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Brza Hrana", monitor, NULL);
    if (!window) return endProgram("Prozor nije uspeo da se kreira.");

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);

    cursor = loadImageToCursor("res/cursor.png");
    glfwSetCursor(window, cursor);

    //glew
    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    //alfa kanal providnosti
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //preprocesing tekstura
    preprocessTexture(bg, "res/background.png");
    preprocessTexture(credentials, "res/credentials.png");
    preprocessTexture(button, "res/button.png");
    preprocessTexture(grill, "res/grill.png");
    preprocessTexture(patty, "res/patty.png");
    preprocessTexture(table, "res/table.png");
    preprocessTexture(plate, "res/plate.png");
    preprocessTexture(lowerBun, "res/lowerBun.png");
    preprocessTexture(burgir, "res/burgir.png");
    preprocessTexture(pickles, "res/pickles.png");
	preprocessTexture(onion, "res/onion.png");
	preprocessTexture(lettuce, "res/lettuce.png");
	preprocessTexture(cheese, "res/cheese.png");
	preprocessTexture(tomato, "res/tomat.png");
	preprocessTexture(upperBun, "res/upperBun.png");
	preprocessTexture(ketchup, "res/ketchup.png");
    preprocessTexture(ketchupSuccess, "res/ketchup_success.png");
    preprocessTexture(ketchupStain, "res/ketchup_stain.png");
    preprocessTexture(mustard, "res/mustard.png");
    preprocessTexture(mustardSuccess, "res/mustard_success.png");
	preprocessTexture(mustardStain, "res/mustard_stain.png");
	preprocessTexture(pozdrav, "res/prijatno.png");
    //verteksi
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
    float grillVertices[] = {
        grillLeft,  grillBottom,  0.0f, 0.0f,
        grillRight, grillBottom,  1.0f, 0.0f,
        grillRight, grillTop,     1.0f, 1.0f,
        grillLeft,  grillBottom,  0.0f, 0.0f,
        grillRight, grillTop,     1.0f, 1.0f,
        grillLeft,  grillTop,     0.0f, 1.0f
    };
    float barBgVertices[] = {
        barLeft,  barBottom,
        barRight, barBottom,
        barRight, barTop,
        barLeft,  barTop
    };
    float tableVertices[] = {
        tableLeft,  tableBottom,  0.0f, 0.0f,
        tableRight, tableBottom,  1.0f, 0.0f,
        tableRight, tableTop,     1.0f, 1.0f,
        tableLeft,  tableBottom,  0.0f, 0.0f,
        tableRight, tableTop,     1.0f, 1.0f,
        tableLeft,  tableTop,     0.0f, 1.0f
    };
    float plateVertices[] = {
        plateLeft,  plateBottom,  0.0f, 0.0f,
        plateRight, plateBottom,  1.0f, 0.0f,
        plateRight, plateTop,     1.0f, 1.0f,
        plateLeft,  plateBottom,  0.0f, 0.0f,
        plateRight, plateTop,     1.0f, 1.0f,
        plateLeft,  plateTop,     0.0f, 1.0f
    };
    float ingredientVertices[] = {
        ingredientLeft,  ingredientBottom,  0.0f, 0.0f,
        ingredientRight, ingredientBottom,  1.0f, 0.0f,
        ingredientRight, ingredientTop,     1.0f, 1.0f,
        ingredientLeft,  ingredientBottom,  0.0f, 0.0f,
        ingredientRight, ingredientTop,     1.0f, 1.0f,
        ingredientLeft,  ingredientTop,     0.0f, 1.0f
    };
    float bottleVertices[] = {
        bottleLeft,  bottleBottom,  0.0f, 0.0f,
        bottleRight, bottleBottom,  1.0f, 0.0f,
        bottleRight, bottleTop,     1.0f, 1.0f,
        bottleLeft,  bottleBottom,  0.0f, 0.0f,
        bottleRight, bottleTop,     1.0f, 1.0f,
        bottleLeft,  bottleTop,     0.0f, 1.0f
	};
    float pozdravVertices[] = {
        pozdravLeft,  pozdravBottom,  0.0f, 0.0f,
        pozdravRight, pozdravBottom,  1.0f, 0.0f,
        pozdravRight, pozdravTop,     1.0f, 1.0f,
        pozdravLeft,  pozdravBottom,  0.0f, 0.0f,
        pozdravRight, pozdravTop,     1.0f, 1.0f,
        pozdravLeft,  pozdravTop,     0.0f, 1.0f
    };

    //VAOs
    unsigned int bgVao;
    unsigned int credVao;
    unsigned int buttonVao;
    unsigned int grillVao;
    unsigned int pattyVao;
    unsigned int barBgVAO; unsigned int barBgVBO;
    unsigned int barFillVAO; unsigned int barFillVBO;
    unsigned int tableVao;
    unsigned int plateVao;
    unsigned int lowerBunVao;
    unsigned int burgirVao;
    unsigned int picklesVao;
    unsigned int onionVao;
    unsigned int lettuceVao;
    unsigned int cheeseVao;
    unsigned int tomatoVao;
    unsigned int upperBunVao;
	unsigned int ketchupVao;
	unsigned int mustardVao;
    unsigned int pozdravVao;

    formVAOs(bgVertices, sizeof(bgVertices), bgVao);
    formVAOs(creditVertices, sizeof(creditVertices), credVao);
    formVAOs(buttonVertices, sizeof(buttonVertices), buttonVao);
    formVAOs(grillVertices, sizeof(grillVertices), grillVao);
    formVAOs(ingredientVertices, sizeof(ingredientVertices), pattyVao);

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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); // 4 floats per vertex
    glEnableVertexAttribArray(0);

    formVAOs(tableVertices, sizeof(tableVertices), tableVao);
    formVAOs(plateVertices, sizeof(plateVertices), plateVao);

    formVAOs(ingredientVertices, sizeof(ingredientVertices), lowerBunVao);
    formVAOs(ingredientVertices, sizeof(ingredientVertices), burgirVao);
    formVAOs(ingredientVertices, sizeof(ingredientVertices), picklesVao);
    formVAOs(ingredientVertices, sizeof(ingredientVertices), onionVao);
    formVAOs(ingredientVertices, sizeof(ingredientVertices), lettuceVao);
    formVAOs(ingredientVertices, sizeof(ingredientVertices), cheeseVao);
    formVAOs(ingredientVertices, sizeof(ingredientVertices), tomatoVao);
    formVAOs(ingredientVertices, sizeof(ingredientVertices), upperBunVao);
	formVAOs(bottleVertices, sizeof(bottleVertices), ketchupVao);
	formVAOs(bottleVertices, sizeof(bottleVertices), mustardVao);
	formVAOs(pozdravVertices, sizeof(pozdravVertices), pozdravVao);

    //shaderi
    unsigned int rectShader = createShader("rect.vert", "rect.frag");
    unsigned int pattyShader = createShader("patty.vert", "patty.frag");
    unsigned int colorShader = createShader("color.vert", "color.frag");
    unsigned int layerShader = createShader("layer.vert", "layer.frag");

    //fps limit
    const double fpsLimit = 1.0 / 75.0;

    std::vector<IngredientInfo> ingredients = {
        { lowerBun, lowerBunVao, 0.1f },
        { burgir,   burgirVao,   0.1f },
        { ketchup, ketchupVao, 0.25f },// -> kecap logika u ovom koraku
	    { mustard, mustardVao, 0.25f },// -> identicna logika kao za kecap ali sa senfom u ovom koraku
        { pickles,  picklesVao,  0.04f },
        { onion,    onionVao,    0.03f },
        { lettuce,  lettuceVao,  0.05f },
        { cheese,   cheeseVao,   0.02f },
        { tomato,   tomatoVao,   0.06f },
        { upperBun, upperBunVao, 0.09f }
    };

    std::vector<Placed> placed;

	//prvi 'prethodni' je tanjir
    Rect prev{ plateLeft, plateRight, plateTop, plateBottom };
    Rect current;

    //glavna petlja
    while (!glfwWindowShouldClose(window))
    {
        double frameStart = glfwGetTime();

        glClear(GL_COLOR_BUFFER_BIT);

        //renderi
        glUseProgram(rectShader);
        glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);
        glBindTexture(GL_TEXTURE_2D, bg);
        glBindVertexArray(bgVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glUseProgram(rectShader);
        glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);
        glBindTexture(GL_TEXTURE_2D, credentials);
        glBindVertexArray(credVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (!buttonClicked) {
            glUseProgram(rectShader);
            glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);
            glBindTexture(GL_TEXTURE_2D, button);
            glBindVertexArray(buttonVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        else {
            //ostatak programa tek nakon sto je dugme kliknuto
            if (cookedness < 1.0f) {
                //pecenje pljeskavice
                glUseProgram(rectShader);
                glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);
                glBindTexture(GL_TEXTURE_2D, grill);
                glBindVertexArray(grillVao);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                glUseProgram(pattyShader);
                glUniform1i(glGetUniformLocation(pattyShader, "uTex"), 0);
                glUniform1f(glGetUniformLocation(pattyShader, "horizontalShift"), shiftHorizontal);
                glUniform1f(glGetUniformLocation(pattyShader, "verticalShift"), shiftVertical);
                glUniform1f(glGetUniformLocation(pattyShader, "cookedness"), cookedness);
                glBindTexture(GL_TEXTURE_2D, patty);
                glBindVertexArray(pattyVao);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                //pozadinski bar
                glUseProgram(colorShader);
                glUniform4f(glGetUniformLocation(colorShader, "uColor"), 1.0f, 1.0f, 1.0f, 1.0f); // white outline
                glBindVertexArray(barBgVAO);
                glDrawArrays(GL_LINE_LOOP, 0, 4);
                //ispunjeni bar
                if (cookedness > 0.0f) {
                    float fillLeft = barLeft + 0.01f; // slight padding inside outline
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
                    //azurira VBO
                    glBindBuffer(GL_ARRAY_BUFFER, barFillVBO);
                    glBufferData(GL_ARRAY_BUFFER, sizeof(barFillVertices), barFillVertices, GL_DYNAMIC_DRAW);
                    //crta zeleni fil
                    glUseProgram(colorShader);
                    glUniform4f(glGetUniformLocation(colorShader, "uColor"), 0.0f, 1.0f, 0.0f, 1.0f);
                    glBindVertexArray(barFillVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }

                Rect pattyRect{ ingredientLeft + shiftHorizontal, ingredientRight + shiftHorizontal, ingredientTop + shiftVertical, ingredientBottom + shiftVertical };
                Rect grillRect{ grillLeft,grillRight,grillTop,grillBottom };
                if (objectsTouching(pattyRect, grillRect)) cookedness += 0.0025f;
            }
            else if (cookedness >= 1.0f && cookedness < 2) {
                //pljeskavica je gotova, resetujemo parametre za sledeci deo
                shiftHorizontal = 0.0f;
                shiftVertical = 0.0f;
                cookedness = 6.7f;
            }
            else {
                if (!ingredients.empty()) {
                    //pravljenje hamburgera (table + plate)
                    glUseProgram(rectShader);
                    glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);
                    glBindTexture(GL_TEXTURE_2D, table);
                    glBindVertexArray(tableVao);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    glUseProgram(rectShader);
                    glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);
                    glBindTexture(GL_TEXTURE_2D, plate);
                    glBindVertexArray(plateVao);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    //svi prethodno postavljeni elementi ostaju fiksirani
                    glUseProgram(layerShader);
                    glUniform1i(glGetUniformLocation(layerShader, "uTex"), 0);
                    for (const auto& p : placed) {
                        glUniform1f(glGetUniformLocation(layerShader, "horizontalShift"), p.hShift);
                        glUniform1f(glGetUniformLocation(layerShader, "verticalShift"), p.vShift);
                        glBindTexture(GL_TEXTURE_2D, p.tex);
                        glBindVertexArray(p.vao);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                    }
                    for (const auto& o : stainsAndSuccesses)
                    {
                        glUniform1f(glGetUniformLocation(layerShader, "horizontalShift"), o.hShift);
                        glUniform1f(glGetUniformLocation(layerShader, "verticalShift"), o.vShift);
                        glBindTexture(GL_TEXTURE_2D, o.tex);
                        glBindVertexArray(o.vao);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                    }

                    unsigned currTex = ingredients[0].texture;

                    bool isKetchup = (currTex == ketchup);
                    bool isMustard = (currTex == mustard);

                    if ((isKetchup || isMustard) && fired)
                    {
                        fired = false;

                        //sredina flase
                        float bottleXLeft = ingredientLeft + shiftHorizontal;
                        float bottleXRight = ingredientRight + shiftHorizontal;
                        float bottleMidX = (bottleXLeft + bottleXRight) / 2.0f;

                        if (placed.empty()) {
                            //ne bi smelo da se desi ali better safe than sorry
                            StickyOverlay stain;
                            stain.tex = isKetchup ? ketchupStain : mustardStain;
                            stain.vao = isKetchup ? ketchupVao : mustardVao;
                            stain.hShift = shiftHorizontal;
                            stain.vShift = (tableTop + tableBottom) / 2.0f;
                            stainsAndSuccesses.push_back(stain);
                            //ne brisemo sastojke i ne stekujemo na klasican nacin
                            shiftHorizontal = shiftVertical = 0.0f;
                            continue;
                        }

                        Rect targetRect = placed.back().rect;

						//gledamo da li je flasa iznad cilja
                        bool hit = (bottleMidX >= targetRect.left && bottleMidX <= targetRect.right);

                        if (hit)
                        {
                            //crtamo preko prethodnog
                            StickyOverlay success;
                            success.tex = isKetchup ? ketchupSuccess : mustardSuccess;
                            success.vao = placed.back().vao;
                            success.hShift = placed.back().hShift;
                            success.vShift = isKetchup ? placed.back().vShift : placed.back().vShift*0.99;
                            stainsAndSuccesses.push_back(success);

                            //sledeci sastojak SAMO ako uspeh
                            ingredients.erase(ingredients.begin());
                            shiftHorizontal = shiftVertical = 0.0f;
                        }
                        else
                        {
                            //promasaj -> mrlja
                            StickyOverlay stain;
                            stain.tex = isKetchup ? ketchupStain : mustardStain;
                            stain.vao = isKetchup ? ketchupVao : mustardVao;
                            stain.hShift = shiftHorizontal;

                            stain.vShift = -1.3f; //ova visina mozda mora da se prilagodi

                            stainsAndSuccesses.push_back(stain);

							//NE brisemo sastojak iz niza, ostaje za dalje pokusaje
                        }

                        continue; //za ovaj korak preskacemo normalni steking
                    }


					//trenutni sastojak koji se pomera tastaturom kao i inace
                    glUniform1f(glGetUniformLocation(layerShader, "horizontalShift"), shiftHorizontal);
                    glUniform1f(glGetUniformLocation(layerShader, "verticalShift"), shiftVertical);
                    glBindTexture(GL_TEXTURE_2D, ingredients[0].texture);
                    glBindVertexArray(ingredients[0].vao);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

					//trenutni rect sastojka sa pomerajem
                    float h = ingredients[0].height;

                    Rect curRect{
                        ingredientLeft + shiftHorizontal,
                        ingredientRight + shiftHorizontal,
                        ingredientTop + shiftVertical,
                        ingredientTop + shiftVertical - h
                    };

					//ako prethonog nema, baza je tanjir
                    Rect baseRect = prev;
                    if (!placed.empty()) {
                        baseRect = placed.back().rect;
                    }

                    if (objectsTouching(baseRect, curRect)) {
                        //flase ne bi smele normalno da se stekuju
                        if (currTex == ketchup || currTex == mustard) {
                            goto skipNormalStacking;
                        }
                        //novi placed je trenutni ingredijent
                        Placed newPlaced;
                        newPlaced.tex = ingredients[0].texture;
                        newPlaced.vao = ingredients[0].vao;
                        newPlaced.hShift = shiftHorizontal;
                        newPlaced.vShift = shiftVertical;
                        newPlaced.rect = curRect;

                        placed.push_back(newPlaced);

                        //apdejtujemo prethodni da bi stek radio
                        prev = newPlaced.rect;

                        // remove this ingredient from the queue
                        ingredients.erase(ingredients.begin());

                        //resetujemo shift za naredni element
                        shiftHorizontal = 0.0f;
                        shiftVertical = 0.0f;
                    }
                    skipNormalStacking:;
                }
                else {
                    //iscrtavamo asemblirani hamburgir
                    glUseProgram(rectShader);
                    glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);
                    glBindTexture(GL_TEXTURE_2D, table);
                    glBindVertexArray(tableVao);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    glBindTexture(GL_TEXTURE_2D, plate);
                    glBindVertexArray(plateVao);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    glUseProgram(layerShader);
                    glUniform1i(glGetUniformLocation(layerShader, "uTex"), 0);

                    for (const auto& p : placed) {
                        glUniform1f(glGetUniformLocation(layerShader, "horizontalShift"), p.hShift);
                        glUniform1f(glGetUniformLocation(layerShader, "verticalShift"), p.vShift);
                        glBindTexture(GL_TEXTURE_2D, p.tex);
                        glBindVertexArray(p.vao);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                    }
                    for (const auto& o : stainsAndSuccesses)
                    {
                        glUniform1f(glGetUniformLocation(layerShader, "horizontalShift"), o.hShift);
                        glUniform1f(glGetUniformLocation(layerShader, "verticalShift"), o.vShift);
                        glBindTexture(GL_TEXTURE_2D, o.tex);
                        glBindVertexArray(o.vao);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                    }

                    //zavrsni gumb
                    glUseProgram(rectShader);
					glUniform1i(glGetUniformLocation(rectShader, "uTex"), 0);
                    glBindTexture(GL_TEXTURE_2D, pozdrav);
                    glBindVertexArray(pozdravVao);
					glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();


        //fps limit
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
