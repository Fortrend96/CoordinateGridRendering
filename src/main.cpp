#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

// Читает текстовый файл целиком.
// Нам это нужно для загрузки GLSL-шейдеров с диска.
static std::string ReadTextFile(const std::string& filePath)
{
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

// Компилирует один шейдер: vertex или fragment.
static GLuint CompileShader(GLenum shaderType, const std::string& source, const std::string& debugName)
{
    GLuint shader = glCreateShader(shaderType);

    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        std::string log;
        log.resize(static_cast<size_t>(logLength));

        glGetShaderInfoLog(shader, logLength, nullptr, log.data());

        glDeleteShader(shader);

        throw std::runtime_error("Failed to compile shader: " + debugName + "\n" + log);
    }

    return shader;
}

// Создаёт shader program из vertex shader и fragment shader.
static GLuint CreateShaderProgram(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
{
    const std::string vertexSource = ReadTextFile(vertexShaderPath);
    const std::string fragmentSource = ReadTextFile(fragmentShaderPath);

    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource, vertexShaderPath);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource, fragmentShaderPath);

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    // После линковки program хранит скомпонованный результат.
    // Отдельные shader objects больше не нужны.
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success != GL_TRUE)
    {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        std::string log;
        log.resize(static_cast<size_t>(logLength));

        glGetProgramInfoLog(program, logLength, nullptr, log.data());

        glDeleteProgram(program);

        throw std::runtime_error("Failed to link shader program\n" + log);
    }

    return program;
}

static void GlfwErrorCallback(int errorCode, const char* description)
{
    std::cerr << "GLFW error " << errorCode << ": " << description << '\n';
}

int main()
{
    glfwSetErrorCallback(GlfwErrorCallback);

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    // Просим OpenGL 4.3 Core Profile.
    // Core Profile означает, что старый fixed-function OpenGL недоступен.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int windowWidth = 1280;
    const int windowHeight = 720;

    GLFWwindow* window = glfwCreateWindow(
        windowWidth,
        windowHeight,
        "Coordinate Grid Rendering",
        nullptr,
        nullptr
    );

    if (window == nullptr)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    // Включаем вертикальную синхронизацию.
    // Это не обязательно, но уменьшает лишнюю нагрузку на GPU.
    glfwSwapInterval(1);

    // GLAD должен быть инициализирован после создания OpenGL-контекста.
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
    std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << '\n';

    std::cout << "Current path: " << std::filesystem::current_path() << '\n';

    // Создаём shader program.
    // Папка shaders копируется рядом с exe через CMake POST_BUILD команду.
    GLuint fullscreenProgram = 0;

    try
    {
        fullscreenProgram = CreateShaderProgram(
            "shaders/fullscreen_triangle.vert",
            "shaders/fullscreen_triangle.frag"
        );
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';

        glfwDestroyWindow(window);
        glfwTerminate();

        return 1;
    }

    // В OpenGL Core Profile нужен VAO даже тогда,
    // когда мы не используем vertex buffer.
    GLuint fullscreenVao = 0;
    glGenVertexArrays(1, &fullscreenVao);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Закрытие окна по Escape.
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        glViewport(0, 0, framebufferWidth, framebufferHeight);

        glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Рисуем fullscreen triangle.
        glUseProgram(fullscreenProgram);
        glBindVertexArray(fullscreenVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &fullscreenVao);
    glDeleteProgram(fullscreenProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}