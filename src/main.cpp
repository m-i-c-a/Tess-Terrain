#include <array>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "Defines.hpp"
#include "Helpers.hpp"

constexpr uint32_t VIEWER_WIDTH = 900u;
constexpr uint32_t VIEWER_HEIGHT = 700u;
constexpr uint32_t NUM_PATCH_PTS = 4u;

enum
{
    PROGRAM_DEFAULT = 0,
    PROGRAM_COUNT
};

enum
{
    TEXTURE_HEIGHTMAP = 0,
    TEXTURE_COUNT
};

enum
{
    VERTEXARRAY_PATCH_TEST = 0,
    VERTEXARRAY_PATCHES = 1,
    VERTEXARRAY_COUNT
};

enum
{
    BUFFER_PATCH_TEST_VERTEX = 0,
    BUFFER_PATCH_VERTEX = 1,
    BUFFER_COUNT
};

struct OpenGLManager
{
    GLuint programs[PROGRAM_COUNT];
    GLuint textures[TEXTURE_COUNT];
    GLuint vertexArrays[VERTEXARRAY_COUNT];
    GLuint buffers[BUFFER_COUNT];
} g_gl;

struct CameraManager
{
    float pitch; // radians
    float yaw;   // radians

    glm::vec3 pos{0.0, 0.0, 50.0};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};

    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
} g_camera;

struct AppManager
{
    size_t heightmap_x_dim = 0;
    size_t heightmap_y_dim = 0;
    size_t test_vertex_count = 0;
    size_t vertex_count = 0;

    bool wireframe = false;
    bool showDebugLOD = false;
    float heightScale = 1.0f;
    int renderType = 0; // 0 = test, 1 = scene
    int minTessLevel = 8;
    int maxTessLevel = 64;
    float minRange = 50.0f;  // Min LOD up to ...
    float maxRange = 500.0f; // Max LOD after ...
} g_app;

void updateCameraMatrix()
{
    g_camera.view = glm::lookAt(g_camera.pos, g_camera.pos + g_camera.forward, {0.0f, 1.0f, 0.0f});
}

/**
 * @brief Recieves cursor position, measured in screen coordinates relative to
 * the top-left corner of the window.
 *
 * @param window active window
 * @param x screen coordinate w.r.t. top left-corner
 * @param y screen coordinate w.r.t. top-left corner
 */
void cursorPosCallback(GLFWwindow *window, double x, double y)
{
    static float x0 = x, y0 = y;
    const float dx = x - x0;
    const float dy = y0 - y;

    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        const static float scalar = 5e-2;
        static float pitch = 0.0f;
        static float yaw = 90.0f;

        pitch += scalar * dy;
        yaw += scalar * dx;

        pitch = std::clamp(pitch, -89.0f, 89.0f);

        g_camera.forward = {
            glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
            glm::sin(glm::radians(pitch)),
            -glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
        };
        g_camera.forward = glm::normalize(g_camera.forward);

        updateCameraMatrix();
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        g_camera.pos.y -= dy;
        updateCameraMatrix();
    }

    x0 = x;
    y0 = y;
}

void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    const float scalar = 3.0f;
    g_camera.pos -= g_camera.forward * static_cast<float>(yoffset) * scalar;

    updateCameraMatrix();
}

GLuint create_texture_2d(const std::string tex_filepath)
{
    GLuint tex_handle;
    glGenTextures(1, &tex_handle);
    glBindTexture(GL_TEXTURE_2D, tex_handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int tex_width, tex_height, tex_num_chan;
    unsigned char *tex_data = stbi_load(tex_filepath.c_str(), &tex_width, &tex_height, &tex_num_chan, 4);

    if (!tex_data)
        EXIT("Failed to load texture " + tex_filepath);

    uint64_t format = 0x0;
    switch (tex_num_chan)
    {
    case 1:
        format = GL_RGBA;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_RGBA;
        break;
    default:
        EXIT("Failed to load texture " + tex_filepath);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, format, tex_width, tex_height, 0, format, GL_UNSIGNED_BYTE, tex_data);

    g_app.heightmap_x_dim = static_cast<size_t>(tex_width);
    g_app.heightmap_y_dim = static_cast<size_t>(tex_height);

    stbi_image_free(tex_data);

    return tex_handle;
}

void init()
{
    g_gl.programs[PROGRAM_DEFAULT] = createProgram("../src/shaders/default.vert", "../src/shaders/default.frag",
                                                   "../src/shaders/tcs.glsl", "../src/shaders/tes.glsl", "DEFAULT");
    g_gl.programs[PROGRAM_DEFAULT] = createProgram("../src/shaders/test_vert.glsl", "../src/shaders/test_frag.glsl",
                                                   "../src/shaders/test_tcs.glsl", "../src/shaders/test_tes.glsl", "DEFAULT");

    g_camera.projection = glm::perspective(glm::radians(45.0f), (float)VIEWER_HEIGHT / (float)VIEWER_WIDTH, 0.1f, 100000.0f);
    updateCameraMatrix();

    size_t heightmap_width = 0;
    size_t heightmap_height = 0;

    // read heightmap
    {
        g_gl.textures[TEXTURE_HEIGHTMAP] = create_texture_2d("../assets/test3.png");
    }

    struct Vertex
    {
        float pos[3];
        float uv[2];

        Vertex()
            : pos{0.0f, 0.0f, 0.0f}, uv{0.0f, 0.0f}
        {
        }

        Vertex(float x, float y, float z, float u, float v)
            : pos{x, y, z}, uv{u, v}
        {
        }
    };

    // create test patch
    {
        constexpr size_t vertex_count = 4;
        std::array<Vertex, vertex_count> vertices;

        constexpr float dim = 5;
        constexpr float half_dim = dim / 2.0f;

        vertices[0] = {-half_dim, -half_dim, 0.0f, 0.0f, 0.0f};
        vertices[1] = { half_dim, -half_dim, 0.0f, 1.0f, 0.0f};
        vertices[2] = {-half_dim,  half_dim, 0.0f, 0.0f, 1.0f};
        vertices[3] = { half_dim,  half_dim, 0.0f, 1.0f, 1.0f};

        glGenVertexArrays(1, &g_gl.vertexArrays[VERTEXARRAY_PATCH_TEST]);
        glGenBuffers(1, &g_gl.buffers[BUFFER_PATCH_TEST_VERTEX]);

        glBindVertexArray(g_gl.vertexArrays[VERTEXARRAY_PATCH_TEST]);

        glBindBuffer(GL_ARRAY_BUFFER, g_gl.buffers[BUFFER_PATCH_TEST_VERTEX]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0u);
        glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));

        glEnableVertexAttribArray(1u);
        glVertexAttribPointer(1u, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));

        glBindVertexArray(0u);
        glBindBuffer(GL_ARRAY_BUFFER, 0u);

        glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);

        g_app.test_vertex_count = vertex_count;
    }

    // create mesh
    {

        constexpr size_t patch_resolution = 20;
        constexpr size_t vertex_count = patch_resolution * patch_resolution * 4;
        std::array<Vertex, vertex_count> vertices;

        const float res = static_cast<float>(patch_resolution);
        const float x_dim = static_cast<float>(g_app.heightmap_x_dim);
        const float y_dim = static_cast<float>(g_app.heightmap_y_dim);
        const float half_x_dim = x_dim / 2.0f;
        const float half_y_dim = y_dim / 2.0f;

        size_t v_idx = 0;
        for (size_t y = 0; y < patch_resolution; ++y)
        {
            for (size_t x = 0; x < patch_resolution; ++x)
            {
                // bottom-left
                vertices[v_idx++] = {
                    -half_x_dim + x_dim * x / res, // x
                    0.0f,                          // y
                    -half_y_dim + y_dim * y / res, // z
                    x / res,                       // u
                    y / res};                      // v

                // bottom-right
                vertices[v_idx++] = {
                    -half_x_dim + x_dim * (x + 1) / res, // x
                    0.0f,                                // y
                    -half_y_dim + y_dim * y / res,       // z
                    (x + 1) / res,                       // u
                    y / res};                            // v

                // top-left
                vertices[v_idx++] = {
                    -half_x_dim + x_dim * x / res,       // x
                    0.0f,                                // y
                    -half_y_dim + y_dim * (y + 1) / res, // z
                    x / res,                             // u
                    (y + 1) / res};                      // v

                // top-right
                vertices[v_idx++] = {
                    -half_x_dim + x_dim * (x + 1) / res, // x
                    0.0f,                                // y
                    -half_y_dim + y_dim * (y + 1) / res, // z
                    (x + 1) / res,                       // u
                    (y + 1) / res};                      // v
            }
        }

        assert(v_idx == vertex_count);

        glGenVertexArrays(1, &g_gl.vertexArrays[VERTEXARRAY_PATCHES]);
        glGenBuffers(1, &g_gl.buffers[BUFFER_PATCH_VERTEX]);

        glBindVertexArray(g_gl.vertexArrays[VERTEXARRAY_PATCHES]);

        glBindBuffer(GL_ARRAY_BUFFER, g_gl.buffers[BUFFER_PATCH_VERTEX]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0u);
        glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));

        glEnableVertexAttribArray(1u);
        glVertexAttribPointer(1u, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));

        glBindVertexArray(0u);
        glBindBuffer(GL_ARRAY_BUFFER, 0u);

        glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);

        g_app.vertex_count = vertex_count;
    }

    {
    std::vector<float> vertices;

    float width = g_app.heightmap_x_dim;
    float height = g_app.heightmap_y_dim;

    unsigned rez = 20;
    for(unsigned i = 0; i <= rez-1; i++)
    {
        for(unsigned j = 0; j <= rez-1; j++)
        {
            vertices.push_back(-width/2.0f + width*i/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*j/(float)rez); // v.z
            vertices.push_back(i / (float)rez); // u
            vertices.push_back(j / (float)rez); // v

            vertices.push_back(-width/2.0f + width*(i+1)/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*j/(float)rez); // v.z
            vertices.push_back((i+1) / (float)rez); // u
            vertices.push_back(j / (float)rez); // v

            vertices.push_back(-width/2.0f + width*i/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*(j+1)/(float)rez); // v.z
            vertices.push_back(i / (float)rez); // u
            vertices.push_back((j+1) / (float)rez); // v

            vertices.push_back(-width/2.0f + width*(i+1)/(float)rez); // v.x
            vertices.push_back(0.0f); // v.y
            vertices.push_back(-height/2.0f + height*(j+1)/(float)rez); // v.z
            vertices.push_back((i+1) / (float)rez); // u
            vertices.push_back((j+1) / (float)rez); // v
        }
    }
    std::cout << "Loaded " << rez*rez << " patches of 4 control points each" << std::endl;
    std::cout << "Processing " << rez*rez*4 << " vertices in vertex shader" << std::endl;

    // first, configure the cube's VAO (and terrainVBO)
    unsigned int terrainVAO, terrainVBO;
    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO);

    glGenBuffers(1, &terrainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);

    g_gl.buffers[BUFFER_PATCH_VERTEX] = terrainVBO;
    g_gl.vertexArrays[VERTEXARRAY_PATCHES] = terrainVAO;
    }
}

void render()
{
    if (g_app.wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearColor(0.12f, 0.63f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(g_gl.programs[PROGRAM_DEFAULT]);
    glBindTexture(GL_TEXTURE_2D, g_gl.textures[TEXTURE_HEIGHTMAP]);

    set_uni_mat4(g_gl.programs[PROGRAM_DEFAULT], "u_projMatrix", g_camera.projection);
    set_uni_mat4(g_gl.programs[PROGRAM_DEFAULT], "u_viewMatrix", g_camera.view);
    set_uni_int(g_gl.programs[PROGRAM_DEFAULT], "u_minTessLevel", g_app.minTessLevel);
    set_uni_int(g_gl.programs[PROGRAM_DEFAULT], "u_maxTessLevel", g_app.maxTessLevel);
    set_uni_float(g_gl.programs[PROGRAM_DEFAULT], "u_minRange", g_app.minRange);
    set_uni_float(g_gl.programs[PROGRAM_DEFAULT], "u_maxRange", g_app.maxRange);
    set_uni_int(g_gl.programs[PROGRAM_DEFAULT], "u_showDebugLOD", g_app.showDebugLOD);
    // set_uni_float(g_gl.programs[PROGRAM_DEFAULT], "u_heightScale", g_app.heightScale);

    if (g_app.renderType == 0)
    {
        // glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(g_gl.vertexArrays[VERTEXARRAY_PATCH_TEST]);
        glDrawArrays(GL_PATCHES, 0, (static_cast<GLsizei>(g_app.test_vertex_count)));
    }
    else
    {
        glBindVertexArray(g_gl.vertexArrays[VERTEXARRAY_PATCHES]);
        glDrawArrays(GL_PATCHES, 0, (static_cast<GLsizei>(g_app.vertex_count)));
    }


    glBindVertexArray(0);
    glUseProgram(0);
}

void release()
{
}

void gui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Gui"))
    {
        ImGui::Checkbox("Wireframe", &g_app.wireframe);
        ImGui::Checkbox("LOD Visual", &g_app.showDebugLOD);
        ImGui::SliderFloat("Height Scale", &g_app.heightScale, 0.1f, 200.0f);

        ImGui::InputInt("Min Tess Lvl", &g_app.minTessLevel);
        ImGui::InputInt("Max Tess Lvl", &g_app.maxTessLevel);
        ImGui::SliderFloat("Min LOD Range", &g_app.minRange, 1.0f, 500.0f);
        ImGui::SliderFloat("Max LOD Range", &g_app.maxRange, 1.0f, 1500.0f);

        ImGui::RadioButton("Test"   , &g_app.renderType, 0); ImGui::SameLine();
        ImGui::RadioButton("Terrain", &g_app.renderType, 1);
    }
    ImGui::End();

    // bool demo = true;
    // ImGui::ShowDemoWindow(&demo);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create the Window
    GLFWwindow *window = glfwCreateWindow(
        VIEWER_WIDTH, VIEWER_HEIGHT,
        "Geometry Clipmaps Demo", nullptr, nullptr);
    if (window == nullptr)
    {
        LOG("=> Failure <=\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, &cursorPosCallback);
    glfwSetScrollCallback(window, &mouseScrollCallback);

    // Load OpenGL functions
    LOG("Loading {OpenGL}\n");
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOG("gladLoadGLLoader failed\n");
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    LOG("-- Begin -- Demo\n");
    LOG("-- Begin -- Init\n");
    init();
    LOG("-- End -- Init\n");

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        render();

        gui();

        glfwSwapBuffers(window);
    }

    release();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}