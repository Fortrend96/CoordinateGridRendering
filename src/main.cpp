#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "OrbitCamera.h"
#include "ShaderProgram.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// Ќабор тестовых режимов сетки.
//
// Ќужен, чтобы быстро провер€ть:
// - обычное положение;
// - большой сдвиг;
// - произвольный поворот;
// - большой сдвиг + поворот.
enum class EGridPreset
{
    Simple = 1,
    LargeOffset = 2,
    Rotated = 3,
    LargeOffsetAndRotated = 4
};

struct SGridParameters
{
    glm::dvec3 vOrigin;
    glm::dvec3 vAxisX;
    glm::dvec3 vAxisY;
    glm::dvec3 vNormal;
};



static void GlfwErrorCallback(int nErrorCode, const char* pszDescription)
{
    std::cerr << "GLFW error " << nErrorCode << ": " << pszDescription << '\n';
}

static void MouseButtonCallback(GLFWwindow* pWindow, int nButton, int nAction, int nMods)
{
    (void)nMods;

    COrbitCamera* pCamera = static_cast<COrbitCamera*>(glfwGetWindowUserPointer(pWindow));

    if (pCamera == nullptr)
    {
        return;
    }

    if (nButton == GLFW_MOUSE_BUTTON_LEFT)
    {
        double dMouseX = 0.0;
        double dMouseY = 0.0;
        glfwGetCursorPos(pWindow, &dMouseX, &dMouseY);

        if (nAction == GLFW_PRESS)
        {
            pCamera->BeginRotation(dMouseX, dMouseY);
        }
        else if (nAction == GLFW_RELEASE)
        {
            pCamera->EndRotation();
        }
    }
}

static void CursorPositionCallback(GLFWwindow* pWindow, double dMouseX, double dMouseY)
{
    COrbitCamera* pCamera = static_cast<COrbitCamera*>(glfwGetWindowUserPointer(pWindow));

    if (pCamera == nullptr)
    {
        return;
    }

    pCamera->UpdateRotation(dMouseX, dMouseY);
}

static void ScrollCallback(GLFWwindow* pWindow, double dOffsetX, double dOffsetY)
{
    (void)dOffsetX;

    COrbitCamera* pCamera = static_cast<COrbitCamera*>(glfwGetWindowUserPointer(pWindow));

    if (pCamera == nullptr)
    {
        return;
    }

    pCamera->AddZoom(dOffsetY);
}

static glm::dmat4 CreateGridRotation(bool bRotated)
{
    if (!bRotated)
    {
        return glm::dmat4(1.0);
    }

    // “естовый произвольный поворот.
    // ќн нужен, чтобы проверить, что сетка работает не только в плоскости Z = 0.
    return
        glm::rotate(glm::dmat4(1.0), glm::radians(25.0), glm::dvec3(1.0, 0.0, 0.0)) *
        glm::rotate(glm::dmat4(1.0), glm::radians(15.0), glm::dvec3(0.0, 1.0, 0.0)) *
        glm::rotate(glm::dmat4(1.0), glm::radians(10.0), glm::dvec3(0.0, 0.0, 1.0));
}

static SGridParameters CreateGridParameters(EGridPreset ePreset)
{
    const bool bLargeOffset =
        ePreset == EGridPreset::LargeOffset ||
        ePreset == EGridPreset::LargeOffsetAndRotated;

    const bool bRotated =
        ePreset == EGridPreset::Rotated ||
        ePreset == EGridPreset::LargeOffsetAndRotated;

    const glm::dvec3 vOrigin = bLargeOffset
        ? glm::dvec3(1000000.0, -2000000.0, 500000.0)
        : glm::dvec3(0.0, 0.0, 0.0);

    const glm::dmat4 mGridRotation = CreateGridRotation(bRotated);

    const glm::dvec3 vAxisX = glm::normalize(
        glm::dvec3(mGridRotation * glm::dvec4(1.0, 0.0, 0.0, 0.0))
    );

    const glm::dvec3 vAxisY = glm::normalize(
        glm::dvec3(mGridRotation * glm::dvec4(0.0, 1.0, 0.0, 0.0))
    );

    const glm::dvec3 vNormal = glm::normalize(glm::cross(vAxisX, vAxisY));

    return SGridParameters
    {
        vOrigin,
        vAxisX,
        vAxisY,
        vNormal
    };
}

static const char* GetGridPresetName(EGridPreset ePreset)
{
    switch (ePreset)
    {
    case EGridPreset::Simple:
        return "Simple";
    case EGridPreset::LargeOffset:
        return "Large offset";
    case EGridPreset::Rotated:
        return "Rotated";
    case EGridPreset::LargeOffsetAndRotated:
        return "Large offset + rotated";
    default:
        return "Unknown";
    }
}

static void PrintControls()
{
    std::cout << '\n';
    std::cout << "Controls:\n";
    std::cout << "  Left mouse button + move : rotate camera\n";
    std::cout << "  Mouse wheel              : zoom\n";
    std::cout << "  R                        : reset camera\n";
    std::cout << "  1                        : simple grid\n";
    std::cout << "  2                        : large offset grid\n";
    std::cout << "  3                        : rotated grid\n";
    std::cout << "  4                        : large offset + rotated grid\n";
    std::cout << "  Esc                      : exit\n";
    std::cout << '\n';
}

int main()
{
    glfwSetErrorCallback(GlfwErrorCallback);

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int nInitialWindowWidth = 1280;
    const int nInitialWindowHeight = 720;

    GLFWwindow* pWindow = glfwCreateWindow(
        nInitialWindowWidth,
        nInitialWindowHeight,
        "Coordinate Grid Rendering",
        nullptr,
        nullptr
    );

    if (pWindow == nullptr)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(pWindow);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(pWindow);
        glfwTerminate();
        return 1;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
    std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << '\n';
    std::cout << "Current path: " << std::filesystem::current_path() << '\n';

    PrintControls();

    CShaderProgram gridShaderProgram;

    try
    {
        gridShaderProgram.LoadFromFiles(
            "shaders/fullscreen_triangle.vert",
            "shaders/grid.frag"
        );
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';

        glfwDestroyWindow(pWindow);
        glfwTerminate();

        return 1;
    }

    GLuint nFullscreenVao = 0;
    glGenVertexArrays(1, &nFullscreenVao);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    EGridPreset eCurrentPreset = EGridPreset::LargeOffsetAndRotated;
    SGridParameters sGridParameters = CreateGridParameters(eCurrentPreset);

    COrbitCamera camera(
        sGridParameters.vOrigin,
        24.0,
        glm::radians(-45.0),
        glm::radians(30.0)
    );

    glfwSetWindowUserPointer(pWindow, &camera);
    glfwSetMouseButtonCallback(pWindow, MouseButtonCallback);
    glfwSetCursorPosCallback(pWindow, CursorPositionCallback);
    glfwSetScrollCallback(pWindow, ScrollCallback);

    auto fnSetPreset = [&](EGridPreset eNewPreset)
        {
            if (eCurrentPreset == eNewPreset)
            {
                return;
            }

            eCurrentPreset = eNewPreset;
            sGridParameters = CreateGridParameters(eCurrentPreset);

            camera.SetTarget(sGridParameters.vOrigin);

            std::cout << "Grid preset: " << GetGridPresetName(eCurrentPreset) << '\n';
        };

    std::cout << "Grid preset: " << GetGridPresetName(eCurrentPreset) << '\n';

    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();

        if (glfwGetKey(pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(pWindow, GLFW_TRUE);
        }

        if (glfwGetKey(pWindow, GLFW_KEY_1) == GLFW_PRESS)
        {
            fnSetPreset(EGridPreset::Simple);
        }
        else if (glfwGetKey(pWindow, GLFW_KEY_2) == GLFW_PRESS)
        {
            fnSetPreset(EGridPreset::LargeOffset);
        }
        else if (glfwGetKey(pWindow, GLFW_KEY_3) == GLFW_PRESS)
        {
            fnSetPreset(EGridPreset::Rotated);
        }
        else if (glfwGetKey(pWindow, GLFW_KEY_4) == GLFW_PRESS)
        {
            fnSetPreset(EGridPreset::LargeOffsetAndRotated);
        }

        if (glfwGetKey(pWindow, GLFW_KEY_R) == GLFW_PRESS)
        {
            camera.Reset(
                sGridParameters.vOrigin,
                24.0,
                glm::radians(-45.0),
                glm::radians(30.0)
            );
        }

        int nFramebufferWidth = 0;
        int nFramebufferHeight = 0;
        glfwGetFramebufferSize(pWindow, &nFramebufferWidth, &nFramebufferHeight);

        glViewport(0, 0, nFramebufferWidth, nFramebufferHeight);

        glClearColor(0.02f, 0.02f, 0.025f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::dmat4 mView = camera.GetViewMatrix();

        const double dAspect =
            nFramebufferHeight > 0
            ? static_cast<double>(nFramebufferWidth) / static_cast<double>(nFramebufferHeight)
            : 1.0;

        const glm::dmat4 mProjection = glm::perspective(
            glm::radians(60.0),
            dAspect,
            0.1,
            1000.0
        );

        const glm::dmat4 mViewProj = mProjection * mView;
        const glm::dmat4 mInvViewProj = glm::inverse(mViewProj);

        gridShaderProgram.Use();

        gridShaderProgram.SetUniformMat4d("uViewProj", mViewProj);
        gridShaderProgram.SetUniformMat4d("uInvViewProj", mInvViewProj);

        gridShaderProgram.SetUniformVec2d(
            "uViewportSize",
            glm::dvec2(
                static_cast<double>(nFramebufferWidth),
                static_cast<double>(nFramebufferHeight)
            )
        );

        gridShaderProgram.SetUniformVec3d("uGridOrigin", sGridParameters.vOrigin);
        gridShaderProgram.SetUniformVec3d("uGridAxisX", sGridParameters.vAxisX);
        gridShaderProgram.SetUniformVec3d("uGridAxisY", sGridParameters.vAxisY);
        gridShaderProgram.SetUniformVec3d("uGridNormal", sGridParameters.vNormal);

        // sin(5∞) ~= 0.087.
        // ≈сли abs(dot(rayDir, normal)) меньше этого порога,
        // значит смотрим на сетку почти с ребра.
        gridShaderProgram.SetUniform1d("uMinViewNormalDot", 0.087);

        gridShaderProgram.SetUniform1d("uMinorStep", 1.0);
        gridShaderProgram.SetUniform1d("uMajorStep", 10.0);

        gridShaderProgram.SetUniform1f("uMinorThickness", 1.0f);
        gridShaderProgram.SetUniform1f("uMajorThickness", 1.5f);
        gridShaderProgram.SetUniform1f("uAxisThickness", 2.5f);

        gridShaderProgram.SetUniformVec4f(
            "uPlaneColor",
            glm::vec4(0.03f, 0.03f, 0.035f, 0.35f)
        );

        gridShaderProgram.SetUniformVec4f(
            "uMinorColor",
            glm::vec4(0.32f, 0.32f, 0.34f, 0.55f)
        );

        gridShaderProgram.SetUniformVec4f(
            "uMajorColor",
            glm::vec4(0.58f, 0.58f, 0.62f, 0.75f)
        );

        gridShaderProgram.SetUniformVec4f(
            "uXAxisColor",
            glm::vec4(0.95f, 0.12f, 0.12f, 0.95f)
        );

        gridShaderProgram.SetUniformVec4f(
            "uYAxisColor",
            glm::vec4(0.15f, 0.85f, 0.20f, 0.95f)
        );

        glBindVertexArray(nFullscreenVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(pWindow);
    }

    glDeleteVertexArrays(1, &nFullscreenVao);

    glfwDestroyWindow(pWindow);
    glfwTerminate();

    return 0;
}