#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
unsigned int loadTexture(const char *path);
unsigned int loadTextureParallax(const char *path, bool gammaCorrection);
unsigned int loadCubemap(vector<std::string> faces);
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
float heightScale = 0.02;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight{
    glm::vec3 position;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight{
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    PointLight pointLight;
    DirLight dirLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // blending
    // -----
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader parallaxMapping("resources/shaders/parallax_mapping.vs", "resources/shaders/parallex_mapping.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader riverShader("resources/shaders/river.vs", "resources/shaders/river.fs");
    Shader grassShader("resources/shaders/grass.vs", "resources/shaders/grass.fs");


    // load models
    // -----------
    Model tree("resources/objects/tree/scene.gltf");
    tree.SetShaderTextureNamePrefix("material.");

    Model bridge("resources/objects/bridge/scene.gltf");
    bridge.SetShaderTextureNamePrefix("material.");

    Model cottage("resources/objects/house/scene.gltf");
    cottage.SetShaderTextureNamePrefix("material.");

    Model trees("resources/objects/trees/scene.gltf");
    trees.SetShaderTextureNamePrefix("material.");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------

    float riverVertices[] = {
            // positions          //normals          // texture Coords
            -15.0f, -1.0f, -3.062f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -15.0f, -1.0f, 1.438f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -13.8f, -1.0f, -3.42156f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -13.8f, -1.0f, 0.95844f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -12.6f, -1.0f, -3.98319f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -12.6f, -1.0f, 0.276814f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -11.4f, -1.0f, -4.55069f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -11.4f, -1.0f, -0.410685f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -10.2f, -1.0f, -4.92581f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -10.2f, -1.0f, -0.905814f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -9.0f, -1.0f, -4.97753f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -9.0f, -1.0f, -1.07753f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -7.8f, -1.0f, -4.68777f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -7.8f, -1.0f, -0.907766f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -6.6f, -1.0f, -4.15775f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -6.6f, -1.0f, -0.497746f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -5.4f, -1.0f, -3.57262f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -5.4f, -1.0f, -0.0326207f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -4.2f, -1.0f, -3.13679f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -4.2f, -1.0f, 0.283209f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -3.0f, -1.0f, -3.00251f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -3.0f, -1.0f, 0.297495f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -1.8f, -1.0f, -3.21667f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -1.8f, -1.0f, -0.0366724f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -0.600002f, -1.0f, -3.70448f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -0.600002f, -1.0f, -0.644479f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            0.599998f, -1.0f, -4.29552f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            0.599998f, -1.0f, -1.35552f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            1.8f, -1.0f, -4.78333f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            1.8f, -1.0f, -1.96333f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            3.0f, -1.0f, -4.99749f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            3.0f, -1.0f, -2.29749f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            4.2f, -1.0f, -4.86321f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            4.2f, -1.0f, -2.28321f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            5.4f, -1.0f, -4.42738f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            5.4f, -1.0f, -1.96738f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            6.6f, -1.0f, -3.84226f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            6.6f, -1.0f, -1.50226f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            7.8f, -1.0f, -3.31223f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            7.8f, -1.0f, -1.09223f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            9.0f, -1.0f, -3.02247f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            9.0f, -1.0f, -0.92247f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            10.2f, -1.0f, -3.07418f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            10.2f, -1.0f, -1.09418f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            11.4f, -1.0f, -3.44931f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            11.4f, -1.0f, -1.58931f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            12.6f, -1.0f, -4.01681f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            12.6f, -1.0f, -2.27681f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            13.8f, -1.0f, -4.57844f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            13.8f, -1.0f, -2.95844f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            15.0f, -1.0f, -4.938f,    0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            15.0f, -1.0f, -3.438f,    0.0f, 1.0f, 0.0f,   0.0f, 1.0f
    };

    unsigned int riverIndices[] ={0, 1, 2,
                                  1, 2, 3,
                                  2, 3, 4,
                                  3, 4, 5,
                                  4, 5, 6,
                                  5, 6, 7,
                                  6, 7, 8,
                                  7, 8, 9,
                                  8, 9, 10,
                                  9, 10, 11,
                                  10, 11, 12,
                                  11, 12, 13,
                                  12, 13, 14,
                                  13, 14, 15,
                                  14, 15, 16,
                                  15, 16, 17,
                                  16, 17, 18,
                                  17, 18, 19,
                                  18, 19, 20,
                                  19, 20, 21,
                                  20, 21, 22,
                                  21, 22, 23,
                                  22, 23, 24,
                                  23, 24, 25,
                                  24, 25, 26,
                                  25, 26, 27,
                                  26, 27, 28,
                                  27, 28, 29,
                                  28, 29, 30,
                                  29, 30, 31,
                                  30, 31, 32,
                                  31, 32, 33,
                                  32, 33, 34,
                                  33, 34, 35,
                                  34, 35, 36,
                                  35, 36, 37,
                                  36, 37, 38,
                                  37, 38, 39,
                                  38, 39, 40,
                                  39, 40, 41,
                                  40, 41, 42,
                                  41, 42, 43,
                                  42, 43, 44,
                                  43, 44, 45,
                                  44, 45, 46,
                                  45, 46, 47,
                                  46, 47, 48,
                                  47, 48, 49,
                                  48, 49, 50,
                                  49, 50, 51
    };


    float grassVertices[] = {
            //postion            // texture coords
            15.0f, -1.01f, 15.0f,  1.0f, 1.0f,
            15.0f, -1.01f, -15.0f, 1.0f, 0.0f,
            -15.0f, -1.01f, -15.0f, 0.0f, 1.0f,
            -15.0f, -1.01f, 15.0f, 0.0f, 0.0f
    };
    unsigned int grassIndices[] = {
            0, 1, 3,
            1, 2, 3
    };

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // grass VAO
    unsigned int grassVBO, grassVAO, grassEBO;
    glGenVertexArrays(1, &grassVAO);
    glGenBuffers(1, &grassVBO);
    glGenBuffers(1, &grassEBO);

    glBindVertexArray(grassVAO);

    glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(grassVertices), grassVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grassEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(grassIndices), grassIndices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // river VAO
    unsigned int riverVBO, riverVAO, riverEBO;
    glGenVertexArrays(1, &riverVAO);
    glGenBuffers(1, &riverVBO);
    glGenBuffers(1, &riverEBO);

    glBindVertexArray(riverVAO);

    glBindBuffer(GL_ARRAY_BUFFER, riverVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(riverVertices), riverVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, riverEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(riverIndices), riverIndices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normals attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // load textures
    // -------------

    unsigned int riverTexture = loadTexture(FileSystem::getPath("resources/textures/river.jpg").c_str());
    unsigned int riverTextureSpec = loadTexture(FileSystem::getPath("resources/textures/river_specular.jpg").c_str());
    unsigned int grassTexture = loadTexture(FileSystem::getPath("resources/textures/skybox/rainbow_dn.png").c_str());

    unsigned int bridgeDiffuse = loadTextureParallax(FileSystem::getPath("resources/objects/bridge/textures/cave_most_01initialShadingGroup1_baseColor.png").c_str(), true);
    unsigned int bridgeSpecular = loadTextureParallax(FileSystem::getPath("resources/objects/bridge/textures/SpecularMap.png").c_str(),false);
    unsigned int bridgeNormal = loadTextureParallax(FileSystem::getPath("resources/objects/bridge/textures/cave_most_01initialShadingGroup1_normal.png").c_str(), false);
    unsigned int bridgeDisMap = loadTextureParallax(FileSystem::getPath("resources/objects/bridge/textures/DisplacementMap.png").c_str(), false);

    vector<std::string> faces
            {

                    FileSystem::getPath("resources/textures/skybox/rainbow_ft.png"),
                    FileSystem::getPath("resources/textures/skybox/rainbow_bk.png"),
                    FileSystem::getPath("resources/textures/skybox/rainbow_up.png"),
                    FileSystem::getPath("resources/textures/skybox/rainbow_dn.png"),
                    FileSystem::getPath("resources/textures/skybox/rainbow_rt.png"),
                    FileSystem::getPath("resources/textures/skybox/rainbow_lf.png")


            };
    unsigned int cubemapTexture = loadCubemap(faces);

    // shader configuration
    // --------------------
    riverShader.use();
    riverShader.setInt("material.diffuse", 0);
    riverShader.setInt("material.specular", 1);

    // point light config
    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(-15.0f, -15.0, 0.0);
    pointLight.ambient = glm::vec3(0.8, 0.6, 0.6);
    pointLight.diffuse = glm::vec3(1.0, 1.0, 1.0);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 10.0f;
    pointLight.linear = 0.09;
    pointLight.quadratic = 0.032f;

    // directionsl light config
    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    dirLight.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    dirLight.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
    dirLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);




    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection/model
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);



        // graw grass
        grassShader.use();
        grassShader.setMat4("projection", projection);
        grassShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        grassShader.setMat4("model", model);
        glBindVertexArray(grassVAO);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        // draw river
        riverShader.use();
        riverShader.setVec3("light.direction", dirLight.direction);
        riverShader.setVec3("viewPos", programState->camera.Position);
        riverShader.setVec3("light.ambient", dirLight.ambient);
        riverShader.setVec3("light.diffuse", dirLight.diffuse);
        riverShader.setVec3("light.specular", dirLight.specular);
        riverShader.setFloat("material.shininess", 32.0f);
        riverShader.setMat4("projection", projection);
        riverShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        riverShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, riverTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, riverTextureSpec);
        glBindVertexArray(riverVAO);
        glDrawElements(GL_TRIANGLES, 52*3, GL_UNSIGNED_INT, 0);
/*
        // parallax mapping
        // ----------------
        parallaxMapping.use();

        parallaxMapping.setVec3("dirLight.direction", dirLight.direction);
        parallaxMapping.setVec3("dirLight.ambient", dirLight.ambient);
        parallaxMapping.setVec3("dirLight.diffuse", dirLight.diffuse);
        parallaxMapping.setVec3("dirLight.specular", dirLight.specular);

        parallaxMapping.setBool("blinn", true);

        parallaxMapping.setVec3("pointLight.position", pointLight.position);
        parallaxMapping.setVec3("pointLight.ambient", pointLight.ambient);
        parallaxMapping.setVec3("pointLight.diffuse", pointLight.diffuse);
        parallaxMapping.setVec3("pointLight.specular", pointLight.specular);
        parallaxMapping.setFloat("pointLight.constant", pointLight.constant);
        parallaxMapping.setFloat("pointLight.linear", pointLight.linear);
        parallaxMapping.setFloat("pointLight.quadratic", pointLight.quadratic);

        parallaxMapping.setVec3("viewPos", programState->camera.Position);
        parallaxMapping.setFloat("material.shininess", 32.0f);

        parallaxMapping.setMat4("projection", projection);
        parallaxMapping.setMat4("view", view);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-3.0f, -0.55f, -1.5f));
        model = glm::scale(model, glm::vec3(0.4f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);

        parallaxMapping.setFloat("heightScale", heightScale);
        parallaxMapping.setInt("material.diffuse", 0);
        parallaxMapping.setInt("material.specular", 1);
        parallaxMapping.setInt("material.normal", 2);
        parallaxMapping.setInt("material.depth", 3);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bridgeDiffuse);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bridgeSpecular);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, bridgeNormal);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, bridgeDisMap);

        renderQuad();
        bridge.Draw(parallaxMapping);

        glDisable(GL_CULL_FACE);
*/
        ourShader.use();

        view = programState->camera.GetViewMatrix();
        projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        model = glm::mat4(1.0f);

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // directional Light
        ourShader.setVec3("dirLight.direction", dirLight.direction);
        ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        ourShader.setVec3("dirLight.specular", dirLight.specular);


        // point light
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        // view/projection transformations
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                      (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glDepthFunc(GL_LEQUAL);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        //trees
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-7.0f, -1.01f, -7.0f));
        model = glm::scale(model, glm::vec3(0.30f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);
        tree.Draw(ourShader);


        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(3.0f, -1.01f, -5.0f));
        model = glm::scale(model, glm::vec3(0.30f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ourShader.setMat4("model", model);
        tree.Draw(ourShader);


        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(10.0f, -1.01f, -7.0f));
        model = glm::scale(model, glm::vec3(0.22f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);
        tree.Draw(ourShader);

        glCullFace(GL_BACK);

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-3.0f, -0.55f, -1.5f));
        model = glm::scale(model, glm::vec3(0.4f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);
        bridge.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-3.0f, -1.01f, -9.0f));
        model = glm::scale(model, glm::vec3(0.0035f));
        ourShader.setMat4("model", model);
        cottage.Draw(ourShader);

        //background trees
        for(int i = 0; i<4; i++){
            model = glm::mat4(1.0f);
            model = glm::translate(model,glm::vec3(-12.0f + i*7.0f, -1.01f, -12.0f));
            model = glm::scale(model, glm::vec3(0.08f - i * 0.01 ));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(i*15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ourShader.setMat4("model", model);
            trees.Draw(ourShader);

            model = glm::mat4(1.0f);
            model = glm::translate(model,glm::vec3(-12.0f, -1.01f, -12.0f + i*7.0f));
            model = glm::scale(model, glm::vec3(0.08f - i * 0.01));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(i*15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ourShader.setMat4("model", model);
            trees.Draw(ourShader);
        }




        glDisable(GL_CULL_FACE);

        // draw skybox as last
        skyboxShader.use();
        skyboxShader.setInt("skybox", 0);

        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS); // set depth function back to default

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteVertexArrays(1, &riverVAO);
    glDeleteBuffers(1, &riverVBO);
    glDeleteBuffers(1, &riverEBO);
    glDeleteVertexArrays(1, &grassVAO);
    glDeleteBuffers(1, &grassVBO);
    glDeleteBuffers(1, &grassEBO);
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int loadTextureParallax(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
            dataFormat = internalFormat = GL_RED;
        else if (nrComponents == 3) {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4) {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGB;
            dataFormat = GL_RGBA;
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// renders a 1x1 quad in NDC with manually calculated tangent vectors
// ------------------------------------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad() {
    if (quadVAO == 0) {
        // positions
        glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3(1.0f, -1.0f, 0.0f);
        glm::vec3 pos4(1.0f, 1.0f, 0.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


        float quadVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z,
                bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z,
                bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z,
                bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z,
                bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z,
                bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z,
                bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *) (6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *) (8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *) (11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}