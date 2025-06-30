#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.hpp"

#include "Shader.hpp"
#include "IO.hpp"
#include "Camera.hpp"
#include "Transform.hpp"
#include "Light.hpp"
#include "Math.hpp"
#include "Model.hpp"
#include "ecs.hpp"
#include "variadicUtils.hpp"
#include "benchmark.cpp"

float deltaTime;
bool firstMouse = true;
glm::vec2 cursorPos;
glm::vec2 cursorPosDelta;

Camera camera(Transform(glm::vec3(0, 0, 3.f)));

static void framebufferSizeCallback(GLFWwindow* window, GLsizei width, GLsizei height)
{
    glViewport(0, 0, width, height);
}

static void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, true);
    const bool running = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    const float spd = (running ? 50.f : 1.f) * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.transform.position += camera.transform.getForward() * spd;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.transform.position -= camera.transform.getForward() * spd;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.transform.position += glm::normalize(glm::cross(camera.transform.getForward(), camera.transform.getUp())) * spd;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.transform.position -= glm::normalize(glm::cross(camera.transform.getForward(), camera.transform.getUp())) * spd;
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    cursorPosDelta.x = firstMouse ? 0 : xpos - cursorPos.x;
    cursorPosDelta.y = firstMouse ? 0 : ypos - cursorPos.y;
    firstMouse = false;
    cursorPos.x = xpos;
    cursorPos.y = ypos;

    const float sensitivity = .5f;
    camera.yaw -= cursorPosDelta.x * sensitivity;
    camera.pitch -= cursorPosDelta.y * sensitivity;
    camera.pitch = camera.pitch > 89.f ? 89.f : camera.pitch;
    camera.pitch = camera.pitch < -89.f ? -89.f : camera.pitch;
    camera.updateTransform();
}

static void windowFocusCallback(GLFWwindow* window, int focused)
{
    firstMouse = true;
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    float sensitivity = 0.5f;
    camera.fov -= yoffset * sensitivity;
}

constexpr size_t iter = 10;
constexpr size_t count = 1000000;
glm::vec3 vec3s[count];
glm::vec2 vec2s[count];
size_t r[count];
ecs::World world{};

static void raw()
{
    for (size_t c = 0; c < iter; c++)
    {
        {
            Benchmark benchmark("raw");
            for (size_t i = 0; i < count; i++)
            {
                r[i] = vec3s[i].x / 1000 + vec3s[i].y / 1000 + vec3s[i].z / 1000;
                r[i] -= vec2s[i].x / 1000 - vec2s[i].y / 1000;
            }
        }
        size_t total = 0;
        for (size_t i = 0; i < count; i++)
            total += r[i];
        std::cout << "total: " << total << "\n";
    }
}
static void test2()
{
    for (size_t c = 0; c < iter; c++)
    {
        {
            Benchmark benchmark("ecs");
            world.executeParallel([](glm::vec3 vec3, glm::vec2 vec2, size_t& rv)
                {
                    rv = vec3.x / 1000 + vec3.y / 1000 + vec3.z / 1000;
                    rv -= vec2.x / 1000 - vec2.y / 1000;
                });
        }
        size_t total = 0;
        world.execute([&total](glm::vec3, glm::vec2, size_t rv)
            {
                total += rv;
            });
        std::cout << "total: " << total << "\n";
    }
}
int main()
{
    for (size_t i = 0; i < count; i++)
    {
        vec3s[i] = { rand(), rand(), rand() };
        vec2s[i] = { rand(), rand() };
        world.add(vec3s[i], vec2s[i], size_t(0));
    }
    raw();
    test2();

    ecs::World world{};
    world.add(12, 'c');
    world.add(glm::vec3(1.5f, 1.5f, 2.5f), glm::vec2(2.7f, 3.7f));
    world.add(glm::vec2(22.7f, 3.7f), glm::vec3(11.5f, 1.5f, 2.5f));
    world.add(glm::vec2(222.7f, 3.7f), glm::vec3(111.5f, 1.5f, 2.5f));
    world.add(glm::vec2(2222.7f, 3.7f), glm::vec3(1111.5f, 1.5f, 2.5f));
    world.add(glm::vec2(22222.7f, 3.7f), glm::vec3(11111.5f, 1.5f, 2.5f), 14);
    std::cout << "count: " << world.totalEntityCount() << " archetypes: " << world.archetypesCount() << "\n";
    world.remove<glm::vec3, glm::vec2>(0);
    world.remove<glm::vec2, glm::vec3>(0);
    std::cout << "count: " << world.totalEntityCount() << " archetypes: " << world.archetypesCount() << "\n";

    auto& archetype = world.getArchetype<glm::vec3, glm::vec2>();
    auto& vec3 = archetype.get<glm::vec3>(0);
    auto& vec2 = archetype.get<glm::vec2>(0);
    std::cout << "vec3: " << vec3.x << " " << vec3.y << " " << vec3.z << "\n";
    std::cout << "vec2: " << vec2.x << " " << vec2.y << "\n";

    world.executeParallel([](glm::vec3& vec3, glm::vec2 vec2)
        {
            std::stringstream msg;
            msg << "vec3: " << vec3.x << " " << vec3.y << " " << vec3.z << " vec2: " << vec2.x << " " << vec2.y << "\n";
            std::cout << msg.str();
            vec3.x = -1;
        });
    world.execute([](ecs::Entity entity, glm::vec2 vec2, glm::vec3 vec3)
        {
            std::cout << "archetype: " << entity.archetypeHash << " rowId: " << entity.rowId << " vec3: " << vec3.x << " " << vec3.y << " " << vec3.z << " vec2: " << vec2.x << " " << vec2.y << "\n";
        });
    world.execute([](int i)
        {
            std::cout << "int: " << i << "\n";
        });
    world.execute([](glm::vec3 v)
        {
            std::cout << "vec3: " << v.x << " " << v.y << " " << v.z << "\n";
        });
    world.execute([](ecs::Entity entity, glm::vec3 v)
        {
            std::cout << "archetype: " << entity.archetypeHash << " rowId: " << entity.rowId << " vec3: " << v.x << " " << v.y << " " << v.z << "\n";
        });

    return 0;

    // configure stbi
    stbi_set_flip_vertically_on_load(true);

    // initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "LeanOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    constexpr int lightsCount = 8;
    Light lights[lightsCount];
    LightBlock lightBlock;
    for (size_t i = 0; i < lightsCount; i++)
    {
        lights[i] = {};
        lights[i].transform.position.x = ((int)i % 4 - 1) * 2.f;
        lights[i].transform.position.y = (((int)i / 4) - 1) * 2.f;
        lights[i].transform.position.z = 1;
        lights[i].transform.lookAt(lights[i].transform.position + glm::vec3(0.f, 1.f, -1.f));
    }

    GLuint ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(lightBlock), &lightBlock, GL_DYNAMIC_DRAW);

    Shader pbrShader(readFile("res/shaders/shader.vert"), readFile("res/shaders/shader.frag"));
    if (!pbrShader)
        return -1;
    Model monkeyModel("res/models/backpack.obj");
    if (!monkeyModel)
        return -1;
    Shader lightShader(readFile("res/shaders/lightShader.vert"), readFile("res/shaders/lightShader.frag"));
    if (!lightShader)
        return -1;
    Model lightModel("res/models/cube.obj");
    if (!lightModel)
        return -1;
    Transform monkeyTransform(glm::vec3(0.f, 0.f, -5), glm::quat(), glm::vec3(4.f));

    // initialize render loop
    glViewport(0, 0, 800, 600);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowFocusCallback(window, windowFocusCallback);
    glfwSetScrollCallback(window, scrollCallback);
    float lastFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        deltaTime = glfwGetTime() - lastFrameTime;
        lastFrameTime = glfwGetTime();

        //std::cout << "frame-time:" << deltaTime << " fps:" << (int)(1 / deltaTime) << "\n";

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // update light block
        for (size_t i = 0; i < lightsCount; i++)
        {
            lights[i].cutoffAngle = lerp(10, 50, sin(lastFrameTime) / 2.f + 0.5f);
            lights[i].outerCutoffAngle = lights[i].cutoffAngle + lerp(1, 45, sin(lastFrameTime * 2) / 2.f + 0.5f);
            lights[i].transform.position.z = sin(lastFrameTime * 2) * 2;
            lightBlock.lights[i].map(lights[i]);
        }
        lightBlock.lightsCount = lightsCount;

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        camera.transform.lookAt(monkeyTransform.position);

        pbrShader.Use();
        pbrShader.SetMat4(pbrShader.GetUniformLocation("projection"), camera.getProjectionMatrix(width, height));
        pbrShader.SetMat4(pbrShader.GetUniformLocation("view"), camera.getViewMatrix());
        pbrShader.SetMat4(pbrShader.GetUniformLocation("model"), monkeyTransform.getMatrix4());
        pbrShader.SetVec3(pbrShader.GetUniformLocation("viewPos"), camera.transform.position);
        pbrShader.SetVec3(pbrShader.GetUniformLocation("material.ambient"), glm::vec3(0.1f));
        pbrShader.SetFloat(pbrShader.GetUniformLocation("material.shininess"), 32);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightBlock), &lightBlock);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
        monkeyModel.draw(pbrShader);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        lightShader.Use();
        lightShader.SetMat4(lightShader.GetUniformLocation("projection"), camera.getProjectionMatrix(width, height));
        lightShader.SetMat4(lightShader.GetUniformLocation("view"), camera.getViewMatrix());

        for (size_t i = 0; i < lightsCount; i++)
        {
            lightShader.SetMat4(lightShader.GetUniformLocation("model"), lights[i].transform.getMatrix4());
            lightShader.SetVec3(lightShader.GetUniformLocation("lightColor"), lights[i].diffuseColor);
            lightModel.draw(lightShader);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}