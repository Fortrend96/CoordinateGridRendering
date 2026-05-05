#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// Читает текстовый файл целиком.
// Используется для загрузки GLSL-шейдеров.
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

// Компилирует один OpenGL-шейдер.
// shaderType может быть GL_VERTEX_SHADER или GL_FRAGMENT_SHADER.
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

    // После линковки program содержит итоговый pipeline-объект.
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

// Небольшая вспомогательная функция для поиска uniform location.
//
// Если location == -1, это не всегда ошибка.
// OpenGL может удалить uniform при оптимизации, если он реально не используется в шейдере.
// Но для отладки предупреждение полезно.
static GLint GetUniformLocation(GLuint program, const char* name)
{
    GLint location = glGetUniformLocation(program, name);

    if (location == -1)
    {
        std::cerr << "Warning: uniform not found: " << name << '\n';
    }

    return location;
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int initialWindowWidth = 1280;
    const int initialWindowHeight = 720;

    GLFWwindow* window = glfwCreateWindow(
        initialWindowWidth,
        initialWindowHeight,
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

    // Вертикальная синхронизация.
    // Не обязательна, но уменьшает нагрузку на GPU.
    glfwSwapInterval(1);

    // GLAD нужно инициализировать только после создания OpenGL-контекста.
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

    GLuint gridProgram = 0;

    try
    {
        gridProgram = CreateShaderProgram(
            "shaders/fullscreen_triangle.vert",
            "shaders/grid.frag"
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
    // когда мы не используем vertex buffers.
    GLuint fullscreenVao = 0;
    glGenVertexArrays(1, &fullscreenVao);

    // Сетка будет писать gl_FragDepth, поэтому включаем depth test.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Включаем alpha blending для полупрозрачной сетки и плоскости.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        glViewport(0, 0, framebufferWidth, framebufferHeight);

        glClearColor(0.02f, 0.02f, 0.025f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---------------------------------------------------------------------
        // Тестовая настройка сетки
        // ---------------------------------------------------------------------
        //
        // Сейчас делаем не только обычную сетку Z = 0,
        // а сразу проверяем более сложный случай:
        //
        // 1. Большое смещение origin относительно мирового нуля.
        // 2. Произвольный поворот плоскости сетки.
        // 3. Отсечение, если смотрим на сетку почти "с ребра".
        //
        // Если хочешь вернуться к простому случаю, поставь:
        //
        // const glm::dvec3 gridOrigin(0.0, 0.0, 0.0);
        // const glm::dmat4 gridRotation(1.0);
        //
        // ---------------------------------------------------------------------

        // Большой сдвиг относительно начала мировых координат.
        // Это нужно, чтобы проверить точность double-вычислений.
        const glm::dvec3 gridOrigin(1000000.0, -2000000.0, 500000.0);

        // Произвольный поворот сетки.
        // Порядок умножения важен: здесь последовательно применяются повороты.
        const glm::dmat4 gridRotation =
            glm::rotate(glm::dmat4(1.0), glm::radians(25.0), glm::dvec3(1.0, 0.0, 0.0)) *
            glm::rotate(glm::dmat4(1.0), glm::radians(15.0), glm::dvec3(0.0, 1.0, 0.0)) *
            glm::rotate(glm::dmat4(1.0), glm::radians(10.0), glm::dvec3(0.0, 0.0, 1.0));

        // Локальная ось X сетки после поворота.
        const glm::dvec3 gridAxisX = glm::normalize(
            glm::dvec3(gridRotation * glm::dvec4(1.0, 0.0, 0.0, 0.0))
        );

        // Локальная ось Y сетки после поворота.
        const glm::dvec3 gridAxisY = glm::normalize(
            glm::dvec3(gridRotation * glm::dvec4(0.0, 1.0, 0.0, 0.0))
        );

        // Нормаль к плоскости сетки.
        // Если axisX и axisY ортонормированы, cross(axisX, axisY) тоже будет unit-length.
        const glm::dvec3 gridNormal = glm::normalize(glm::cross(gridAxisX, gridAxisY));

        // ---------------------------------------------------------------------
        // Камера
        // ---------------------------------------------------------------------
        //
        // Камеру ставим рядом с gridOrigin, а не рядом с мировым нулём.
        // Иначе при большом gridOrigin сетка окажется далеко вне поля зрения.
        //
        // Положение камеры выбрано относительно сетки:
        // немного по X, немного по Y, немного вверх.
        //
        const glm::dvec3 cameraTarget = gridOrigin;
        const glm::dvec3 cameraPosition = gridOrigin + glm::dvec3(12.0, -12.0, 8.0);
        const glm::dvec3 cameraUp(0.0, 0.0, 1.0);

        const glm::dmat4 view = glm::lookAt(cameraPosition, cameraTarget, cameraUp);

        const double aspect =
            framebufferHeight > 0
            ? static_cast<double>(framebufferWidth) / static_cast<double>(framebufferHeight)
            : 1.0;

        const glm::dmat4 projection = glm::perspective(
            glm::radians(60.0),
            aspect,
            0.1,
            1000.0
        );

        // OpenGL convention:
        // clip = projection * view * worldPosition
        const glm::dmat4 viewProj = projection * view;
        const glm::dmat4 invViewProj = glm::inverse(viewProj);

        glUseProgram(gridProgram);

        glUniformMatrix4dv(
            GetUniformLocation(gridProgram, "uViewProj"),
            1,
            GL_FALSE,
            glm::value_ptr(viewProj)
        );

        glUniformMatrix4dv(
            GetUniformLocation(gridProgram, "uInvViewProj"),
            1,
            GL_FALSE,
            glm::value_ptr(invViewProj)
        );

        glUniform2d(
            GetUniformLocation(gridProgram, "uViewportSize"),
            static_cast<double>(framebufferWidth),
            static_cast<double>(framebufferHeight)
        );

        glUniform3d(
            GetUniformLocation(gridProgram, "uGridOrigin"),
            gridOrigin.x,
            gridOrigin.y,
            gridOrigin.z
        );

        glUniform3d(
            GetUniformLocation(gridProgram, "uGridAxisX"),
            gridAxisX.x,
            gridAxisX.y,
            gridAxisX.z
        );

        glUniform3d(
            GetUniformLocation(gridProgram, "uGridAxisY"),
            gridAxisY.x,
            gridAxisY.y,
            gridAxisY.z
        );

        glUniform3d(
            GetUniformLocation(gridProgram, "uGridNormal"),
            gridNormal.x,
            gridNormal.y,
            gridNormal.z
        );

        // Порог отсечения при взгляде почти с ребра.
        //
        // abs(dot(rayDir, normal)) близко к 0 означает, что луч почти параллелен плоскости.
        // sin(5°) примерно равно 0.087.
        //
        // То есть при угле меньше примерно 5 градусов к плоскости
        // сетку не рисуем.
        glUniform1d(
            GetUniformLocation(gridProgram, "uMinViewNormalDot"),
            0.087
        );

        // Малый шаг: 1 единица.
        // Большой шаг: 10 единиц.
        glUniform1d(GetUniformLocation(gridProgram, "uMinorStep"), 1.0);
        glUniform1d(GetUniformLocation(gridProgram, "uMajorStep"), 10.0);

        // Толщины линий.
        glUniform1f(GetUniformLocation(gridProgram, "uMinorThickness"), 1.0f);
        glUniform1f(GetUniformLocation(gridProgram, "uMajorThickness"), 1.5f);
        glUniform1f(GetUniformLocation(gridProgram, "uAxisThickness"), 2.5f);

        // Цвет полупрозрачной плоскости сетки.
        glUniform4f(
            GetUniformLocation(gridProgram, "uPlaneColor"),
            0.03f,
            0.03f,
            0.035f,
            0.35f
        );

        // Малые линии сетки.
        glUniform4f(
            GetUniformLocation(gridProgram, "uMinorColor"),
            0.32f,
            0.32f,
            0.34f,
            0.55f
        );

        // Большие линии сетки.
        glUniform4f(
            GetUniformLocation(gridProgram, "uMajorColor"),
            0.58f,
            0.58f,
            0.62f,
            0.75f
        );

        // Ось X — красная.
        glUniform4f(
            GetUniformLocation(gridProgram, "uXAxisColor"),
            0.95f,
            0.12f,
            0.12f,
            0.95f
        );

        // Ось Y — зелёная.
        glUniform4f(
            GetUniformLocation(gridProgram, "uYAxisColor"),
            0.15f,
            0.85f,
            0.20f,
            0.95f
        );

        glBindVertexArray(fullscreenVao);

        // Рисуем один fullscreen triangle.
        // Вся сетка появляется внутри fragment shader.
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &fullscreenVao);
    glDeleteProgram(gridProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}