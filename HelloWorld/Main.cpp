#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include "Shader.hpp"
#include "IO.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Camera.hpp"
#include "Transform.hpp"
#include "Light.hpp"
#include "Math.hpp"

float deltaTime;
bool firstMouse = true;
glm::vec2 cursorPos;
glm::vec2 cursorPosDelta;

Camera camera(Transform(glm::vec3(0, 0, 3.f)));

static void printCameraPos()
{
    auto forward = camera.transform.getForward();
    auto up = camera.transform.getUp();
    auto pos = camera.transform.position;
    std::cout << "pos: " << pos.x << " " << pos.y << " " << pos.z << " for: " << forward.x << " " << forward.y << " " << forward.z << " up: " << up.x << " " << up.y << " " << up.z << "\n";
}

static void framebufferSizeCallback(GLFWwindow* window, GLsizei width, GLsizei height)
{
    glViewport(0, 0, width, height);
}

static void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, true);
    const bool running = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    const float spd = (running ? 5.f : 1.f) * deltaTime;
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

    printCameraPos();
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

GLfloat vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

GLfloat texCoords[] = {
    0.5f, 0.5f,
    0.5f, 0.f,
    -0.5f, 0.f,
};

Transform cubePositions[] = {
    Transform(glm::vec3(-1.f, 0.f, 0.f)),
    Transform(glm::vec3(0.f, 0.f, 0.f)),
    Transform(glm::vec3(1.f, 0.f, 0.f)),
    Transform(glm::vec3(0.0f,  0.0f,  0.0f)),
    Transform(glm::vec3(2.0f,  5.0f, -15.0f)),
    Transform(glm::vec3(-1.5f, -2.2f, -2.5f)),
    Transform(glm::vec3(-3.8f, -2.0f, -12.3f)),
    Transform(glm::vec3(2.4f, -0.4f, -3.5f)),
    Transform(glm::vec3(-1.7f,  3.0f, -7.5f)),
    Transform(glm::vec3(1.3f, -2.0f, -2.5f)),
    Transform(glm::vec3(1.5f,  2.0f, -2.5f)),
    Transform(glm::vec3(1.5f,  0.2f, -1.5f)),
    Transform(glm::vec3(-1.3f,  1.0f, -1.5f))
};

int main()
{
    printCameraPos();

    // initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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

    Shader cubeShader(readFile("shader.vert"), readFile("shader.frag"));
    if (!cubeShader)
        return -1;

    stbi_set_flip_vertically_on_load(true);
    GLuint texture1;
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height, nrChannels;
    stbi_uc* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
    if (!data)
    {
        std::cerr << "failed to load image \"container.jpg\"" << std::endl;
        return -1;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    GLuint texture2;
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    data = stbi_load("awesomeface.png", &width, &height, &nrChannels, 0);
    if (!data)
    {
        std::cerr << "failed to load image \"awesomeface.png\"" << std::endl;
        return -1;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    cubeShader.Use();
    cubeShader.SetInt(cubeShader.GetUniformLocation("tex1"), 0);
    cubeShader.SetInt(cubeShader.GetUniformLocation("tex2"), 1);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), NULL);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // light
    GLuint lightVao;
    glGenVertexArrays(1, &lightVao);
    glBindVertexArray(lightVao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), NULL);
    glEnableVertexAttribArray(0);
    Shader lightShader(readFile("lightShader.vert"), readFile("lightShader.frag"));
    Light light(glm::vec3(0.f, 2.f, 0.f), glm::vec3(1.f, 0.f, 0.f));

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

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // cubes
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);
        cubeShader.Use();
        glfwGetWindowSize(window, &width, &height);
        cubeShader.SetMat4(cubeShader.GetUniformLocation("projection"), camera.getProjectionMatrix(width, height));
        cubeShader.SetMat4(cubeShader.GetUniformLocation("view"), camera.getViewMatrix());
        cubeShader.SetVec3(cubeShader.GetUniformLocation("lightColor"), light.color);
        cubeShader.SetVec4(cubeShader.GetUniformLocation("objectColor"), glm::vec4(1));
        cubeShader.SetVec3(cubeShader.GetUniformLocation("lightPos"), light.transform.position);
        cubeShader.SetVec3(cubeShader.GetUniformLocation("viewPos"), camera.transform.position);
        for (size_t i = 0; i < sizeof(cubePositions) / sizeof(Transform); i++)
        {
            cubePositions[i].rotateAround(glm::vec3(1.f, 0.f, 0.f), .5f * deltaTime * (i + 1));
            cubePositions[i].rotateAround(glm::vec3(0.f, 1.f, 0.f), .5f * deltaTime * (i + 1));
            cubePositions[i].rotateAround(glm::vec3(0.f, 1.f, 1.f), .5f * deltaTime * (i + 1));
            cubeShader.SetMat4(cubeShader.GetUniformLocation("model"), cubePositions[i].getMatrix4());
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // light
        glBindVertexArray(lightVao);
        light.transform.position.x = glm::sin(lastFrameTime) * 10;
        light.transform.position.y = glm::cos(lastFrameTime) * 10;
        lightShader.Use();
        lightShader.SetVec3(cubeShader.GetUniformLocation("lightColor"), light.color);
        lightShader.SetMat4(cubeShader.GetUniformLocation("projection"), camera.getProjectionMatrix(width, height));
        lightShader.SetMat4(cubeShader.GetUniformLocation("view"), camera.getViewMatrix());
        lightShader.SetMat4(lightShader.GetUniformLocation("model"), light.transform.getMatrix4());
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
}