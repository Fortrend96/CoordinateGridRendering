#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "AxisMarkerRenderer.h"
#include "GridRenderer.h"
#include "OrbitCamera.h"
#include "ShaderProgram.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

// Набор тестовых режимов сетки.
//
// Нужен, чтобы быстро проверять:
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

// Состояние приложения, которое нужно callback-функциям GLFW.
//
// Callback'ам нужен доступ:
// - к камере;
// - к текущей геометрии сетки;
// - к текущему режиму проекции.
//
// Это нужно для:
// - pan / orbit / zoom;
// - reset камеры на текущую сетку;
// - zoom в направлении курсора мыши.
struct SApplicationState
{
    COrbitCamera* pCamera;
    SGridGeometry* pGridGeometry;

    bool* pUseOrthographicProjection;

    double dDefaultCameraDistance;
    double dDefaultCameraYawRadians;
    double dDefaultCameraPitchRadians;
};

static void GlfwErrorCallback(int nErrorCode, const char* pszDescription)
{
    std::cerr << "GLFW error " << nErrorCode << ": " << pszDescription << '\n';
}

static glm::dvec2 GetCursorNdc(GLFWwindow* pWindow)
{
    double dCursorX = 0.0;
    double dCursorY = 0.0;

    glfwGetCursorPos(pWindow, &dCursorX, &dCursorY);

    int nWindowWidth = 0;
    int nWindowHeight = 0;

    glfwGetWindowSize(pWindow, &nWindowWidth, &nWindowHeight);

    if (nWindowWidth <= 0 || nWindowHeight <= 0)
    {
        return glm::dvec2(0.0, 0.0);
    }

    // GLFW cursor coordinates:
    // x — слева направо
    // y — сверху вниз
    //
    // OpenGL NDC:
    // x — слева направо [-1; 1]
    // y — снизу вверх [-1; 1]
    const double dNdcX =
        dCursorX / static_cast<double>(nWindowWidth) * 2.0 - 1.0;

    const double dNdcY =
        1.0 - dCursorY / static_cast<double>(nWindowHeight) * 2.0;

    return glm::dvec2(dNdcX, dNdcY);
}

static glm::dmat4 CreateProjectionMatrix(
    bool bUseOrthographicProjection,
    double dAspect
)
{
    if (bUseOrthographicProjection)
    {
        // Тестовый ортографический режим.
        //
        // Размер подобран так, чтобы сетка была видна примерно
        // в том же масштабе, что и в перспективе.
        const double dHalfHeight = 18.0;
        const double dHalfWidth = dHalfHeight * dAspect;

        return glm::ortho(
            -dHalfWidth,
            dHalfWidth,
            -dHalfHeight,
            dHalfHeight,
            0.1,
            1000.0
        );
    }

    return glm::perspective(
        glm::radians(60.0),
        dAspect,
        0.1,
        1000.0
    );
}

static bool TryGetCursorGridPoint(
    GLFWwindow* pWindow,
    const COrbitCamera& camera,
    const SGridGeometry& sGridGeometry,
    bool bUseOrthographicProjection,
    glm::dvec3& vResult
)
{
    int nFramebufferWidth = 0;
    int nFramebufferHeight = 0;

    glfwGetFramebufferSize(pWindow, &nFramebufferWidth, &nFramebufferHeight);

    const double dAspect =
        nFramebufferHeight > 0
        ? static_cast<double>(nFramebufferWidth) / static_cast<double>(nFramebufferHeight)
        : 1.0;

    const glm::dmat4 mView = camera.GetViewMatrix();

    const glm::dmat4 mProjection = CreateProjectionMatrix(
        bUseOrthographicProjection,
        dAspect
    );

    const glm::dmat4 mViewProj = mProjection * mView;
    const glm::dmat4 mInvViewProj = glm::inverse(mViewProj);

    const glm::dvec2 vNdc = GetCursorNdc(pWindow);

    // OpenGL NDC:
    // near plane = -1
    // far plane  =  1
    const glm::dvec4 vNearClip(vNdc.x, vNdc.y, -1.0, 1.0);
    const glm::dvec4 vFarClip(vNdc.x, vNdc.y, 1.0, 1.0);

    glm::dvec4 vNearWorld = mInvViewProj * vNearClip;
    glm::dvec4 vFarWorld = mInvViewProj * vFarClip;

    vNearWorld /= vNearWorld.w;
    vFarWorld /= vFarWorld.w;

    const glm::dvec3 vRayOrigin = glm::dvec3(vNearWorld);
    const glm::dvec3 vRayDirection = glm::normalize(
        glm::dvec3(vFarWorld - vNearWorld)
    );

    const double dDenom = glm::dot(vRayDirection, sGridGeometry.vNormal);

    if (std::abs(dDenom) < 1e-12)
    {
        return false;
    }

    const double dT = glm::dot(
        sGridGeometry.vOrigin - vRayOrigin,
        sGridGeometry.vNormal
    ) / dDenom;

    if (dT < 0.0)
    {
        return false;
    }

    vResult = vRayOrigin + vRayDirection * dT;

    return true;
}

static void MouseButtonCallback(GLFWwindow* pWindow, int nButton, int nAction, int nMods)
{
    (void)nMods;

    SApplicationState* pApplicationState =
        static_cast<SApplicationState*>(glfwGetWindowUserPointer(pWindow));

    if (pApplicationState == nullptr || pApplicationState->pCamera == nullptr)
    {
        return;
    }

    COrbitCamera* pCamera = pApplicationState->pCamera;

    double dMouseX = 0.0;
    double dMouseY = 0.0;

    glfwGetCursorPos(pWindow, &dMouseX, &dMouseY);

    if (nButton == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        if (nAction == GLFW_PRESS)
        {
            // Детектор двойного клика средней кнопкой.
            //
            // glfwGetTime() возвращает время в секундах с момента запуска GLFW.
            // Если два нажатия произошли быстрее заданного интервала,
            // считаем это double click.
            static double s_dLastMiddleClickTime = -1.0;

            const double dCurrentTime = glfwGetTime();
            const double dDoubleClickInterval = 0.35;

            const bool bIsDoubleClick =
                s_dLastMiddleClickTime > 0.0 &&
                (dCurrentTime - s_dLastMiddleClickTime) <= dDoubleClickInterval;

            s_dLastMiddleClickTime = dCurrentTime;

            if (bIsDoubleClick)
            {
                // Пока это не полноценный zoom extents.
                // Но поведение близкое по смыслу: возвращаем камеру
                // к текущей сетке и смотрим на её origin.
                if (pApplicationState->pGridGeometry != nullptr)
                {
                    pCamera->Reset(
                        pApplicationState->pGridGeometry->vOrigin,
                        pApplicationState->dDefaultCameraDistance,
                        pApplicationState->dDefaultCameraYawRadians,
                        pApplicationState->dDefaultCameraPitchRadians
                    );

                    std::cout << "Camera reset by middle mouse double click\n";
                }

                return;
            }

            const bool bShiftPressed =
                glfwGetKey(pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(pWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

            if (bShiftPressed)
            {
                // AutoCAD-like: Shift + middle mouse drag -> orbit.
                pCamera->BeginOrbit(dMouseX, dMouseY);
            }
            else
            {
                // AutoCAD-like: middle mouse drag -> pan.
                pCamera->BeginPan(dMouseX, dMouseY);
            }
        }
        else if (nAction == GLFW_RELEASE)
        {
            pCamera->EndDrag();
        }
    }

    // Дополнительно оставляем старое управление:
    // ЛКМ + движение мыши тоже вращает камеру.
    if (nButton == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (nAction == GLFW_PRESS)
        {
            pCamera->BeginOrbit(dMouseX, dMouseY);
        }
        else if (nAction == GLFW_RELEASE)
        {
            pCamera->EndDrag();
        }
    }
}

static void CursorPositionCallback(GLFWwindow* pWindow, double dMouseX, double dMouseY)
{
    SApplicationState* pApplicationState =
        static_cast<SApplicationState*>(glfwGetWindowUserPointer(pWindow));

    if (pApplicationState == nullptr || pApplicationState->pCamera == nullptr)
    {
        return;
    }

    pApplicationState->pCamera->UpdateDrag(dMouseX, dMouseY);
}

static void ScrollCallback(GLFWwindow* pWindow, double dOffsetX, double dOffsetY)
{
    (void)dOffsetX;

    SApplicationState* pApplicationState =
        static_cast<SApplicationState*>(glfwGetWindowUserPointer(pWindow));

    if (
        pApplicationState == nullptr ||
        pApplicationState->pCamera == nullptr ||
        pApplicationState->pGridGeometry == nullptr ||
        pApplicationState->pUseOrthographicProjection == nullptr
        )
    {
        return;
    }

    COrbitCamera* pCamera = pApplicationState->pCamera;
    const SGridGeometry& sGridGeometry = *pApplicationState->pGridGeometry;
    const bool bUseOrthographicProjection =
        *pApplicationState->pUseOrthographicProjection;

    glm::dvec3 vGridPointBeforeZoom(0.0);

    const bool bHasPointBeforeZoom = TryGetCursorGridPoint(
        pWindow,
        *pCamera,
        sGridGeometry,
        bUseOrthographicProjection,
        vGridPointBeforeZoom
    );

    // Сначала меняем расстояние камеры.
    pCamera->AddZoom(dOffsetY);

    if (!bHasPointBeforeZoom)
    {
        return;
    }

    glm::dvec3 vGridPointAfterZoom(0.0);

    const bool bHasPointAfterZoom = TryGetCursorGridPoint(
        pWindow,
        *pCamera,
        sGridGeometry,
        bUseOrthographicProjection,
        vGridPointAfterZoom
    );

    if (!bHasPointAfterZoom)
    {
        return;
    }

    // Сдвигаем target так, чтобы точка, которая была под курсором до зума,
    // осталась под курсором после зума.
    const glm::dvec3 vTargetCorrection =
        vGridPointBeforeZoom - vGridPointAfterZoom;

    pCamera->SetTarget(pCamera->GetTarget() + vTargetCorrection);
}

static glm::dmat4 CreateGridRotation(bool bRotated)
{
    if (!bRotated)
    {
        return glm::dmat4(1.0);
    }

    // Тестовый произвольный поворот.
    //
    // Он нужен, чтобы проверить, что сетка работает не только
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

    // Локальная ось X сетки после поворота.
    const glm::dvec3 vAxisX = glm::normalize(
        glm::dvec3(mGridRotation * glm::dvec4(1.0, 0.0, 0.0, 0.0))
    );

    // Локальная ось Y сетки после поворота.
    const glm::dvec3 vAxisY = glm::normalize(
        glm::dvec3(mGridRotation * glm::dvec4(0.0, 1.0, 0.0, 0.0))
    );

    // Нормаль к плоскости сетки.
    //
    // Пока считаем, что оси X и Y ортонормированы.
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
    std::cout << "  Middle mouse button + move         : pan\n";
    std::cout << "  Middle mouse double click          : reset camera to current grid\n";
    std::cout << "  Shift + middle mouse button + move : orbit\n";
    std::cout << "  Left mouse button + move           : orbit fallback\n";
    std::cout << "  Mouse wheel                        : zoom to cursor\n";
    std::cout << "  R                                  : reset camera\n";
    std::cout << "  P                                  : toggle perspective/orthographic projection\n";
    std::cout << "  B                                  : toggle infinite/bounded grid\n";
    std::cout << "  M                                  : toggle lines/dots mode\n";
    std::cout << "  C                                  : toggle depth clamp\n";
    std::cout << "  1                                  : simple grid\n";
    std::cout << "  2                                  : large offset grid\n";
    std::cout << "  3                                  : rotated grid\n";
    std::cout << "  4                                  : large offset + rotated grid\n";
    std::cout << "  Esc                                : exit\n";
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

    // Просим OpenGL 4.3 Core Profile.
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

    // Вертикальная синхронизация.
    glfwSwapInterval(1);

    // GLAD нужно инициализировать только после создания OpenGL-контекста.
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
    CShaderProgram axisMarkerShaderProgram;

    try
    {
        gridShaderProgram.LoadFromFiles(
            "shaders/fullscreen_triangle.vert",
            "shaders/grid.frag"
        );

        axisMarkerShaderProgram.LoadFromFiles(
            "shaders/axis_marker.vert",
            "shaders/axis_marker.frag"
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

    CAxisMarkerRenderer axisMarkerRenderer;
    axisMarkerRenderer.Initialize();

    SGridStyle sGridStyle = gridRenderer.GetStyle();
    gridRenderer.SetStyle(sGridStyle);

    // Сетка пишет gl_FragDepth, поэтому включаем depth test.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Включаем alpha blending для полупрозрачной сетки и плоскости.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    EGridPreset eCurrentPreset = EGridPreset::LargeOffsetAndRotated;
    SGridGeometry sGridGeometry = CreateGridGeometry(eCurrentPreset);

    const double dDefaultCameraDistance = 24.0;
    const double dDefaultCameraYawRadians = glm::radians(-45.0);
    const double dDefaultCameraPitchRadians = glm::radians(30.0);

    COrbitCamera camera(
        sGridGeometry.vOrigin,
        dDefaultCameraDistance,
        dDefaultCameraYawRadians,
        dDefaultCameraPitchRadians
    );

    bool bUseOrthographicProjection = false;

    SApplicationState sApplicationState;
    sApplicationState.pCamera = &camera;
    sApplicationState.pGridGeometry = &sGridGeometry;
    sApplicationState.pUseOrthographicProjection = &bUseOrthographicProjection;
    sApplicationState.dDefaultCameraDistance = dDefaultCameraDistance;
    sApplicationState.dDefaultCameraYawRadians = dDefaultCameraYawRadians;
    sApplicationState.dDefaultCameraPitchRadians = dDefaultCameraPitchRadians;

    glfwSetWindowUserPointer(pWindow, &sApplicationState);
    glfwSetMouseButtonCallback(pWindow, MouseButtonCallback);
    glfwSetCursorPosCallback(pWindow, CursorPositionCallback);
    glfwSetScrollCallback(pWindow, ScrollCallback);

    bool bWasPPressed = false;
    bool bWasBPressed = false;
    bool bWasMPressed = false;
    bool bWasCPressed = false;

    auto fnSetPreset = [&](EGridPreset eNewPreset)
        {
            if (eCurrentPreset == eNewPreset)
            {
                return;
            }

            eCurrentPreset = eNewPreset;
            sGridGeometry = CreateGridGeometry(eCurrentPreset);

            // При переключении режима переносим цель камеры в origin новой сетки.
            camera.SetTarget(sGridGeometry.vOrigin);

            std::cout << "Grid preset: " << GetGridPresetName(eCurrentPreset) << '\n';
        };

    std::cout << "Grid preset: " << GetGridPresetName(eCurrentPreset) << '\n';
    std::cout << "Projection: perspective\n";
    std::cout << "Grid bounds: " << (sGridStyle.bIsBounded ? "bounded" : "infinite") << '\n';
    std::cout << "Grid mode: " << (sGridStyle.bDrawDots ? "dots" : "lines") << '\n';
    std::cout << "Depth clamp: " << (sGridStyle.bClampDepth ? "enabled" : "disabled") << '\n';

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
                dDefaultCameraDistance,
                dDefaultCameraYawRadians,
                dDefaultCameraPitchRadians
            );
        }

        // Переключение перспективной/ортографической проекции.
        const bool bIsPPressed = glfwGetKey(pWindow, GLFW_KEY_P) == GLFW_PRESS;

        if (bIsPPressed && !bWasPPressed)
        {
            bUseOrthographicProjection = !bUseOrthographicProjection;

            std::cout << "Projection: "
                << (bUseOrthographicProjection ? "orthographic" : "perspective")
                << '\n';
        }

        bWasPPressed = bIsPPressed;

        // Переключение infinite/bounded grid.
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

        // Переключение режима lines/dots.
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

        // Переключение depth clamp.
        const bool bIsCPressed = glfwGetKey(pWindow, GLFW_KEY_C) == GLFW_PRESS;

        if (bIsCPressed && !bWasCPressed)
        {
            sGridStyle.bClampDepth = !sGridStyle.bClampDepth;
            gridRenderer.SetStyle(sGridStyle);

            std::cout << "Depth clamp: "
                << (sGridStyle.bClampDepth ? "enabled" : "disabled")
                << '\n';
        }

        bWasCPressed = bIsCPressed;

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

        const glm::dmat4 mProjection = CreateProjectionMatrix(
            bUseOrthographicProjection,
            dAspect
        );

        // OpenGL convention:
        //
        // clip = projection * view * worldPosition
        const glm::dmat4 mViewProj = mProjection * mView;
        const glm::dmat4 mInvViewProj = glm::inverse(mViewProj);

        SGridFrameData sFrameData;
        sFrameData.mView = mView;
        sFrameData.mViewProj = mViewProj;
        sFrameData.mInvViewProj = mInvViewProj;
        sFrameData.vViewportSize = glm::dvec2(
            static_cast<double>(nFramebufferWidth),
            static_cast<double>(nFramebufferHeight)
        );

        gridRenderer.SetGeometry(sGridGeometry);
        gridRenderer.Render(gridShaderProgram, sFrameData);

        axisMarkerRenderer.Render(
            axisMarkerShaderProgram,
            sFrameData,
            sGridGeometry
        );

        glfwSwapBuffers(pWindow);
    }

    // Явный Destroy делает порядок освобождения ресурсов очевидным.
    axisMarkerRenderer.Destroy();
    gridRenderer.Destroy();

    glfwDestroyWindow(pWindow);
    glfwTerminate();

    return 0;
}