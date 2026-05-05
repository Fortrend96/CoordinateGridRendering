#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GridRenderer.h"
#include "OrbitCamera.h"
#include "ShaderProgram.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

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
    //
    // ќн нужен, чтобы проверить, что сетка работает не только
    // в простой плоскости Z = 0.
    return
        glm::rotate(glm::dmat4(1.0), glm::radians(25.0), glm::dvec3(1.0, 0.0, 0.0)) *
        glm::rotate(glm::dmat4(1.0), glm::radians(15.0), glm::dvec3(0.0, 1.0, 0.0)) *
        glm::rotate(glm::dmat4(1.0), glm::radians(10.0), glm::dvec3(0.0, 0.0, 1.0));
}

static SGridGeometry CreateGridGeometry(EGridPreset ePreset)
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

    // Ћокальна€ ось X сетки после поворота.
    const glm::dvec3 vAxisX = glm::normalize(
        glm::dvec3(mGridRotation * glm::dvec4(1.0, 0.0, 0.0, 0.0))
    );

    // Ћокальна€ ось Y сетки после поворота.
    const glm::dvec3 vAxisY = glm::normalize(
        glm::dvec3(mGridRotation * glm::dvec4(0.0, 1.0, 0.0, 0.0))
    );

    // Ќормаль к плоскости сетки.
    //
    // ѕока считаем, что оси X и Y ортонормированы.
    const glm::dvec3 vNormal = glm::normalize(glm::cross(vAxisX, vAxisY));

    SGridGeometry sGeometry;
    sGeometry.vOrigin = vOrigin;
    sGeometry.vAxisX = vAxisX;
    sGeometry.vAxisY = vAxisY;
    sGeometry.vNormal = vNormal;

    return sGeometry;
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
    std::cout << "  B                        : toggle infinite/bounded grid\n";
    std::cout << "  M                        : toggle lines/dots mode\n";
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

    // ѕросим OpenGL 4.3 Core Profile.
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

    // ¬ертикальна€ синхронизаци€.
    glfwSwapInterval(1);

    // GLAD нужно инициализировать только после создани€ OpenGL-контекста.
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

    CGridRenderer gridRenderer;
    gridRenderer.Initialize();

    SGridStyle sGridStyle = gridRenderer.GetStyle();

    // —етка пишет gl_FragDepth, поэтому включаем depth test.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // ¬ключаем alpha blending дл€ полупрозрачной сетки и плоскости.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    EGridPreset eCurrentPreset = EGridPreset::LargeOffsetAndRotated;
    SGridGeometry sGridGeometry = CreateGridGeometry(eCurrentPreset);

    COrbitCamera camera(
        sGridGeometry.vOrigin,
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
            sGridGeometry = CreateGridGeometry(eCurrentPreset);

            // ѕри переключении режима переносим цель камеры в origin новой сетки.
            camera.SetTarget(sGridGeometry.vOrigin);

            std::cout << "Grid preset: " << GetGridPresetName(eCurrentPreset) << '\n';
        };

    std::cout << "Grid preset: " << GetGridPresetName(eCurrentPreset) << '\n';

    bool bWasBPressed = false;
    bool bWasMPressed = false;

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
                sGridGeometry.vOrigin,
                24.0,
                glm::radians(-45.0),
                glm::radians(30.0)
            );
        }

        const bool bIsBPressed = glfwGetKey(pWindow, GLFW_KEY_B) == GLFW_PRESS;

        if (bIsBPressed && !bWasBPressed)
        {
            sGridStyle.bIsBounded = !sGridStyle.bIsBounded;
            gridRenderer.SetStyle(sGridStyle);

            std::cout << "Grid bounds: "
                << (sGridStyle.bIsBounded ? "bounded" : "infinite")
                << '\n';
        }

        bWasBPressed = bIsBPressed;

        const bool bIsMPressed = glfwGetKey(pWindow, GLFW_KEY_M) == GLFW_PRESS;

        if (bIsMPressed && !bWasMPressed)
        {
            sGridStyle.bDrawDots = !sGridStyle.bDrawDots;
            gridRenderer.SetStyle(sGridStyle);

            std::cout << "Grid mode: "
                << (sGridStyle.bDrawDots ? "dots" : "lines")
                << '\n';
        }

        bWasMPressed = bIsMPressed;

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

        // OpenGL convention:
        //
        // clip = projection * view * worldPosition
        const glm::dmat4 mViewProj = mProjection * mView;
        const glm::dmat4 mInvViewProj = glm::inverse(mViewProj);

        SGridFrameData sFrameData;
        sFrameData.mViewProj = mViewProj;
        sFrameData.mInvViewProj = mInvViewProj;
        sFrameData.vViewportSize = glm::dvec2(
            static_cast<double>(nFramebufferWidth),
            static_cast<double>(nFramebufferHeight)
        );

        gridRenderer.SetGeometry(sGridGeometry);
        gridRenderer.Render(gridShaderProgram, sFrameData);

        glfwSwapBuffers(pWindow);
    }

    // CGridRenderer сам удалит VAO в деструкторе,
    // но €вный Destroy делает пор€док освобождени€ ресурсов очевидным.
    gridRenderer.Destroy();

    glfwDestroyWindow(pWindow);
    glfwTerminate();

    return 0;
}