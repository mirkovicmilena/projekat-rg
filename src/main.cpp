#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float sensitivity = 0.1; // gloval variable used to reduce mouse sensitivity

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader riverShader("resources/shaders/river.vs", "resources/shaders/river.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------

    float river_vertices[] = {
            // positions          //normals          // texture Coords
            -2.5f, -0.879462f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -2.5f, -0.329462f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -2.3f, -0.896846f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -2.3f, -0.366846f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -2.1f, -0.835788f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -2.1f, -0.325788f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -1.9f, -0.705929f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -1.9f, -0.215929f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -1.7f, -0.52777f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -1.7f, -0.0577704f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -1.5f, -0.32944f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -1.5f, 0.12056f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -1.3f, -0.142249f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -1.3f, 0.287751f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -1.1f, 0.0042484f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -1.1f, 0.414248f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -0.9f, 0.0869239f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -0.9f, 0.476924f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -0.7f, 0.0927248f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -0.7f, 0.462725f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -0.5f, 0.0207353f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -0.5f, 0.370735f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            -0.3f, -0.117679f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            -0.3f, 0.212321f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            -0.0999997f, -0.300666f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            -0.0999997f, 0.00933435f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            0.1f, -0.499335f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            0.1f, -0.209335f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            0.3f, -0.682321f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            0.3f, -0.412322f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            0.5f, -0.820736f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            0.5f, -0.570736f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            0.7f, -0.892725f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            0.7f, -0.662725f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            0.9f, -0.886924f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            0.9f, -0.676924f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            1.1f, -0.804248f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            1.1f, -0.614248f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            1.3f, -0.65775f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            1.3f, -0.48775f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            1.5f, -0.47056f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            1.5f, -0.32056f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            1.7f, -0.272229f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            1.7f, -0.142229f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            1.9f, -0.0940707f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            1.9f, 0.0159293f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            2.1f, 0.0357881f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            2.1f, 0.125788f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            2.3f, 0.0968456f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
            2.3f, 0.166846f, -1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
            2.5f, 0.079462f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
            2.5f, 0.129462f, -1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
    };

    unsigned int river_indices[] ={0, 1, 2,
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




    // river VAO
    unsigned int riverVBO, riverVAO, riverEBO;
    glGenVertexArrays(1, &riverVAO);
    glGenBuffers(1, &riverVBO);
    glGenBuffers(1, &riverEBO);

    glBindVertexArray(riverVAO);

    glBindBuffer(GL_ARRAY_BUFFER, riverVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(river_vertices), river_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, riverEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(river_indices), river_indices, GL_STATIC_DRAW);

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

    unsigned int river_texture = loadTexture(FileSystem::getPath("resources/textures/river.jpg").c_str());
    unsigned int river_texture_spec = loadTexture(FileSystem::getPath("resources/textures/river_specular.jpg").c_str());

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

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
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
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw scene as normal

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        // draw river

        // be sure to activate shader when setting uniforms/drawing objects
        riverShader.use();
        riverShader.setVec3("light.direction", -0.2f, -1.0f, -0.3f);
        riverShader.setVec3("viewPos", camera.Position);

        // light properties
        riverShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
        riverShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
        riverShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

        // material properties
        riverShader.setFloat("material.shininess", 32.0f);

        // view/projection transformations

        riverShader.setMat4("projection", projection);
        riverShader.setMat4("view", view);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        //model = glm::rotate(model, glm::radians(90), glm::vec3(1.0f, 0.0f, 0.0f));
        riverShader.setMat4("model", model);

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, river_texture);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, river_texture_spec);

        glBindVertexArray(riverVAO);
        glDrawElements(GL_TRIANGLES, 52*3, GL_UNSIGNED_INT, 0);


        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    //reducing sensitivity
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
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
