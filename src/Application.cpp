#include "Application.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

CApplication::CApplication()
    : m_pWindow(nullptr)
    , m_nInitialWindowWidth(1280)
    , m_nInitialWindowHeight(720)
    , m_eCurrentPreset(EGridPreset::Simple)
    , m_eCameraViewMode(ECameraViewMode::Top)
    , m_bUseOrthographicProjection(true)
    , m_dDefaultCameraDistance(24.0)
    , m_dDefaultCameraYawRadians(0.0)
    , m_dDefaultCameraPitchRadians(0.0)
    , m_bWasPPressed(false)
    , m_bWasBPressed(false)
    , m_bWasMPressed(false)
    , m_bWasCPressed(false)
    , m_bWasFPressed(false)
    , m_bWasGPressed(false)
    , m_bWasF1Pressed(false)
    , m_bWasF2Pressed(false)
    , m_bWasF3Pressed(false)
    , m_bWasF4Pressed(false)
    , m_bWasF5Pressed(false)
{
    const SCameraViewModeSettings sInitialViewSettings =
        GetCameraViewModeSettings(m_eCameraViewMode);

    m_bUseOrthographicProjection = sInitialViewSettings.bUseOrthographicProjection;
    m_dDefaultCameraYawRadians = sInitialViewSettings.dYawRadians;
    m_dDefaultCameraPitchRadians = sInitialViewSettings.dPitchRadians;
}

CApplication::~CApplication()
{
    Shutdown();
}

int CApplication::Run()
{
    InitializeGlfw();
    CreateWindow();
    InitializeOpenGl();
    LoadShaders();
    InitializeScene();
    RegisterCallbacks();

    MainLoop();

    return 0;
}

void CApplication::InitializeGlfw()
{
    glfwSetErrorCallback(GlfwErrorCallback);

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void CApplication::CreateWindow()
{
    m_pWindow = glfwCreateWindow(
        m_nInitialWindowWidth,
        m_nInitialWindowHeight,
        "Coordinate Grid Rendering",
        nullptr,
        nullptr
    );

    if (m_pWindow == nullptr)
    {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_pWindow);
    glfwSwapInterval(1);
}

void CApplication::InitializeOpenGl()
{
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
    std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << '\n';
    std::cout << "Current path: " << std::filesystem::current_path() << '\n';

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void CApplication::LoadShaders()
{
    m_gridShaderProgram.LoadFromFiles(
        "shaders/fullscreen_quad.vert",
        "shaders/grid.frag"
    );

    m_axisMarkerShaderProgram.LoadFromFiles(
        "shaders/axis_marker.vert",
        "shaders/axis_marker.frag"
    );

    m_sceneObjectShaderProgram.LoadFromFiles(
        "shaders/object.vert",
        "shaders/object.frag"
    );
}

void CApplication::InitializeScene()
{
    PrintControls();

    m_gridRenderer.Initialize();
    m_axisMarkerRenderer.Initialize();
    m_demoSceneRenderer.Initialize();

    m_sGridStyle = m_gridRenderer.GetStyle();
    m_gridRenderer.SetStyle(m_sGridStyle);

    m_eCurrentPreset = EGridPreset::Simple;
    m_sGridGeometry = CreateGridGeometry(m_eCurrentPreset);

    m_pCamera = std::make_unique<COrbitCamera>(
        m_sGridGeometry.vOrigin,
        m_dDefaultCameraDistance,
        m_dDefaultCameraYawRadians,
        m_dDefaultCameraPitchRadians
    );

    ApplyCameraViewMode(m_eCameraViewMode);

    PrintInitialState();
}

void CApplication::RegisterCallbacks()
{
    glfwSetWindowUserPointer(m_pWindow, this);
    glfwSetMouseButtonCallback(m_pWindow, MouseButtonCallback);
    glfwSetCursorPosCallback(m_pWindow, CursorPositionCallback);
    glfwSetScrollCallback(m_pWindow, ScrollCallback);
}

void CApplication::MainLoop()
{
    while (!glfwWindowShouldClose(m_pWindow))
    {
        glfwPollEvents();

        ProcessInput();
        RenderFrame();

        glfwSwapBuffers(m_pWindow);
    }
}

void CApplication::ProcessInput()
{
    if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_pWindow, GLFW_TRUE);
    }

    if (glfwGetKey(m_pWindow, GLFW_KEY_1) == GLFW_PRESS)
    {
        SetGridPreset(EGridPreset::Simple);
    }
    else if (glfwGetKey(m_pWindow, GLFW_KEY_2) == GLFW_PRESS)
    {
        SetGridPreset(EGridPreset::LargeOffset);
    }
    else if (glfwGetKey(m_pWindow, GLFW_KEY_3) == GLFW_PRESS)
    {
        SetGridPreset(EGridPreset::Rotated);
    }
    else if (glfwGetKey(m_pWindow, GLFW_KEY_4) == GLFW_PRESS)
    {
        SetGridPreset(EGridPreset::LargeOffsetAndRotated);
    }

    if (glfwGetKey(m_pWindow, GLFW_KEY_R) == GLFW_PRESS && m_pCamera != nullptr)
    {
        ApplyCameraViewMode(m_eCameraViewMode);
    }

    const bool bIsF1Pressed = glfwGetKey(m_pWindow, GLFW_KEY_F1) == GLFW_PRESS;

    if (bIsF1Pressed && !m_bWasF1Pressed)
    {
        ApplyCameraViewMode(ECameraViewMode::Top);
    }

    m_bWasF1Pressed = bIsF1Pressed;

    const bool bIsF2Pressed = glfwGetKey(m_pWindow, GLFW_KEY_F2) == GLFW_PRESS;

    if (bIsF2Pressed && !m_bWasF2Pressed)
    {
        ApplyCameraViewMode(ECameraViewMode::Front);
    }

    m_bWasF2Pressed = bIsF2Pressed;

    const bool bIsF3Pressed = glfwGetKey(m_pWindow, GLFW_KEY_F3) == GLFW_PRESS;

    if (bIsF3Pressed && !m_bWasF3Pressed)
    {
        ApplyCameraViewMode(ECameraViewMode::Right);
    }

    m_bWasF3Pressed = bIsF3Pressed;

    const bool bIsF4Pressed = glfwGetKey(m_pWindow, GLFW_KEY_F4) == GLFW_PRESS;

    if (bIsF4Pressed && !m_bWasF4Pressed)
    {
        ApplyCameraViewMode(ECameraViewMode::IsometricOrthographic);
    }

    m_bWasF4Pressed = bIsF4Pressed;

    const bool bIsF5Pressed = glfwGetKey(m_pWindow, GLFW_KEY_F5) == GLFW_PRESS;

    if (bIsF5Pressed && !m_bWasF5Pressed)
    {
        ApplyCameraViewMode(ECameraViewMode::Perspective);
    }

    m_bWasF5Pressed = bIsF5Pressed;

    const bool bIsPPressed = glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS;

    if (bIsPPressed && !m_bWasPPressed)
    {
        m_bUseOrthographicProjection = !m_bUseOrthographicProjection;

        std::cout << "Projection: "
            << (m_bUseOrthographicProjection ? "orthographic" : "perspective")
            << '\n';
    }

    m_bWasPPressed = bIsPPressed;

    const bool bIsBPressed = glfwGetKey(m_pWindow, GLFW_KEY_B) == GLFW_PRESS;

    if (bIsBPressed && !m_bWasBPressed)
    {
        m_sGridStyle.bIsBounded = !m_sGridStyle.bIsBounded;
        m_gridRenderer.SetStyle(m_sGridStyle);

        std::cout << "Grid bounds: "
            << (m_sGridStyle.bIsBounded ? "bounded" : "infinite")
            << '\n';
    }

    m_bWasBPressed = bIsBPressed;

    const bool bIsMPressed = glfwGetKey(m_pWindow, GLFW_KEY_M) == GLFW_PRESS;

    if (bIsMPressed && !m_bWasMPressed)
    {
        m_sGridStyle.bDrawDots = !m_sGridStyle.bDrawDots;
        m_gridRenderer.SetStyle(m_sGridStyle);

        std::cout << "Grid mode: "
            << (m_sGridStyle.bDrawDots ? "dots" : "lines")
            << '\n';
    }

    m_bWasMPressed = bIsMPressed;

    const bool bIsCPressed = glfwGetKey(m_pWindow, GLFW_KEY_C) == GLFW_PRESS;

    if (bIsCPressed && !m_bWasCPressed)
    {
        m_sGridStyle.bClampDepth = !m_sGridStyle.bClampDepth;
        m_gridRenderer.SetStyle(m_sGridStyle);

        std::cout << "Depth clamp: "
            << (m_sGridStyle.bClampDepth ? "enabled" : "disabled")
            << '\n';
    }

    m_bWasCPressed = bIsCPressed;

    const bool bIsFPressed = glfwGetKey(m_pWindow, GLFW_KEY_F) == GLFW_PRESS;

    if (bIsFPressed && !m_bWasFPressed)
    {
        m_sGridStyle.bDrawPlane = !m_sGridStyle.bDrawPlane;
        m_gridRenderer.SetStyle(m_sGridStyle);

        std::cout << "Grid plane fill: "
            << (m_sGridStyle.bDrawPlane ? "enabled" : "disabled")
            << '\n';
    }

    m_bWasFPressed = bIsFPressed;

    const bool bIsGPressed = glfwGetKey(m_pWindow, GLFW_KEY_G) == GLFW_PRESS;

    if (bIsGPressed && !m_bWasGPressed)
    {
        m_sGridStyle.bDebugDepthZones = !m_sGridStyle.bDebugDepthZones;
        m_gridRenderer.SetStyle(m_sGridStyle);

        std::cout << "Depth zones debug: "
            << (m_sGridStyle.bDebugDepthZones ? "enabled" : "disabled")
            << '\n';
    }

    m_bWasGPressed = bIsGPressed;
}

void CApplication::RenderFrame()
{
    int nFramebufferWidth = 0;
    int nFramebufferHeight = 0;

    glfwGetFramebufferSize(
        m_pWindow,
        &nFramebufferWidth,
        &nFramebufferHeight
    );

    glViewport(0, 0, nFramebufferWidth, nFramebufferHeight);

    // -------------------------------------------------------------------------
    // 1. Очищаем framebuffer.
    //
    // Цвет фона выбран в CAD-like тёмной гамме.
    //
    // Важно:
    // depth buffer тоже обязательно очищаем, потому что в текущем кадре
    // модельные объекты должны заново записать свою глубину.
    // -------------------------------------------------------------------------
    glClearColor(0.145f, 0.176f, 0.223f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_pCamera == nullptr)
    {
        return;
    }

    // -------------------------------------------------------------------------
    // 2. Формируем матрицы текущего кадра.
    //
    // View matrix берётся из OrbitCamera.
    // Projection matrix зависит от текущего режима:
    // - orthographic;
    // - perspective.
    // -------------------------------------------------------------------------
    const glm::dmat4 mView = m_pCamera->GetViewMatrix();

    const double dAspect =
        nFramebufferHeight > 0
        ? static_cast<double>(nFramebufferWidth) /
        static_cast<double>(nFramebufferHeight)
        : 1.0;

    double dNearPlane = 0.1;
    double dFarPlane = 1000.0;

    CalculateCameraClippingPlanes(
        dNearPlane,
        dFarPlane
    );

    const glm::dmat4 mProjection = CreateProjectionMatrix(
        dAspect,
        dNearPlane,
        dFarPlane
    );

    // OpenGL convention:
    //
    // clip = projection * view * worldPosition
    const glm::dmat4 mViewProj = mProjection * mView;

    // Обратная view-projection нужна сетке:
    // fragment shader восстанавливает луч текущего пикселя через gl_FragCoord.
    const glm::dmat4 mInvViewProj = glm::inverse(mViewProj);

    // -------------------------------------------------------------------------
    // 3. Заполняем общие данные кадра.
    //
    // Эти данные используются:
    // - рендерером сетки;
    // - рендерером маркера;
    // - рендерером тестовых объектов.
    // -------------------------------------------------------------------------
    SGridFrameData sFrameData;

    sFrameData.mView = mView;
    sFrameData.mViewProj = mViewProj;
    sFrameData.mInvViewProj = mInvViewProj;

    sFrameData.vViewportSize = glm::dvec2(
        static_cast<double>(nFramebufferWidth),
        static_cast<double>(nFramebufferHeight)
    );

    sFrameData.bIsOrthographicProjection = m_bUseOrthographicProjection;

    // -------------------------------------------------------------------------
    // 4. Рисуем модельные объекты тестовой сцены.
    //
    // Это именно тестовые объекты для проверки взаимодействия сетки со сценой.
    //
    // Объекты должны:
    // - проходить depth test;
    // - писать глубину в depth buffer;
    // - рисоваться без blending, потому что они непрозрачные.
    //
    // После этого сетка будет рисоваться поверх той же сцены,
    // но с включённым Z-test и отключённой записью глубины.
    // -------------------------------------------------------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDepthMask(GL_TRUE);

    glDisable(GL_BLEND);

    m_demoSceneRenderer.Render(
        m_sceneObjectShaderProgram,
        sFrameData,
        m_sGridGeometry
    );

    // -------------------------------------------------------------------------
    // 5. Рисуем аналитическую сетку.
    //
    // Сетка должна:
    // - использовать тот же view/projection;
    // - участвовать в depth test;
    // - НЕ писать глубину в depth buffer;
    // - использовать blending, потому что линии полупрозрачные.
    //
    // Такое поведение нужно для CAD-like результата:
    // - модельные объекты могут перекрывать сетку;
    // - сетка не портит depth buffer для последующих проходов;
    // - gl_FragDepth сетки проверяется относительно уже нарисованной сцены.
    // -------------------------------------------------------------------------
    m_gridRenderer.SetGeometry(m_sGridGeometry);

    // Adaptive step пересчитывается каждый кадр, потому что масштаб сетки
    // зависит от текущей камеры, projection matrix и viewport size.
    m_gridRenderer.UpdateAdaptiveStep(sFrameData);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDepthMask(GL_FALSE);

    m_gridRenderer.Render(
        m_gridShaderProgram,
        sFrameData
    );

    glDepthMask(GL_TRUE);

    // -------------------------------------------------------------------------
    // 6. Рисуем маркер начала координат.
    //
    // Маркер пока оставляем как отдельную вспомогательную сущность демо.
    // Он не является частью логики сетки.
    //
    // Сейчас он рисуется после сетки, чтобы оставаться читаемым.
    // -------------------------------------------------------------------------
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_axisMarkerRenderer.Render(
        m_axisMarkerShaderProgram,
        sFrameData,
        m_sGridGeometry
    );

    // -------------------------------------------------------------------------
    // 7. Возвращаем безопасные OpenGL-состояния.
    //
    // Это полезно, чтобы следующий кадр начинался из ожидаемого состояния.
    // -------------------------------------------------------------------------
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}

void CApplication::Shutdown()
{
    m_demoSceneRenderer.Destroy();
    m_axisMarkerRenderer.Destroy();
    m_gridRenderer.Destroy();

    if (m_pWindow != nullptr)
    {
        glfwDestroyWindow(m_pWindow);
        m_pWindow = nullptr;
    }

    glfwTerminate();
}

void CApplication::CalculateCameraClippingPlanes(
    double& dNearPlane,
    double& dFarPlane
) const
{
    const double dCameraDistance = m_pCamera != nullptr
        ? m_pCamera->GetDistance()
        : m_dDefaultCameraDistance;

    dNearPlane = std::clamp(
        dCameraDistance * 0.0005,
        0.00001,
        0.1
    );

    dFarPlane = std::max(
        1000.0,
        dCameraDistance * 50.0
    );

    if (dFarPlane < dNearPlane * 1000.0)
    {
        dFarPlane = dNearPlane * 1000.0;
    }
}

glm::dmat4 CApplication::CreateProjectionMatrix(
    double dAspect,
    double dNearPlane,
    double dFarPlane
) const
{
    const double dOrthographicHalfHeight =
        m_pCamera != nullptr
        ? m_pCamera->GetDistance()
        : m_dDefaultCameraDistance;

    if (m_bUseOrthographicProjection)
    {
        const double dSafeHalfHeight = std::clamp(
            dOrthographicHalfHeight,
            0.0001,
            1000000000.0
        );

        const double dHalfWidth = dSafeHalfHeight * dAspect;

        return glm::ortho(
            -dHalfWidth,
            dHalfWidth,
            -dSafeHalfHeight,
            dSafeHalfHeight,
            dNearPlane,
            dFarPlane
        );
    }

    return glm::perspective(
        glm::radians(60.0),
        dAspect,
        dNearPlane,
        dFarPlane
    );
}

glm::dvec2 CApplication::GetCursorNdc() const
{
    double dCursorX = 0.0;
    double dCursorY = 0.0;

    glfwGetCursorPos(m_pWindow, &dCursorX, &dCursorY);

    int nWindowWidth = 0;
    int nWindowHeight = 0;

    glfwGetWindowSize(m_pWindow, &nWindowWidth, &nWindowHeight);

    if (nWindowWidth <= 0 || nWindowHeight <= 0)
    {
        return glm::dvec2(0.0, 0.0);
    }

    const double dNdcX =
        dCursorX / static_cast<double>(nWindowWidth) * 2.0 - 1.0;

    const double dNdcY =
        1.0 - dCursorY / static_cast<double>(nWindowHeight) * 2.0;

    return glm::dvec2(dNdcX, dNdcY);
}

bool CApplication::TryGetCursorGridPoint(glm::dvec3& vResult) const
{
    if (m_pCamera == nullptr)
    {
        return false;
    }

    int nFramebufferWidth = 0;
    int nFramebufferHeight = 0;

    glfwGetFramebufferSize(
        m_pWindow,
        &nFramebufferWidth,
        &nFramebufferHeight
    );

    const double dAspect =
        nFramebufferHeight > 0
        ? static_cast<double>(nFramebufferWidth) / static_cast<double>(nFramebufferHeight)
        : 1.0;

    double dNearPlane = 0.1;
    double dFarPlane = 1000.0;

    CalculateCameraClippingPlanes(
        dNearPlane,
        dFarPlane
    );

    const glm::dmat4 mView = m_pCamera->GetViewMatrix();

    const glm::dmat4 mProjection = CreateProjectionMatrix(
        dAspect,
        dNearPlane,
        dFarPlane
    );

    const glm::dmat4 mViewProj = mProjection * mView;
    const glm::dmat4 mInvViewProj = glm::inverse(mViewProj);

    const glm::dvec2 vNdc = GetCursorNdc();

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

    const double dDenom =
        glm::dot(vRayDirection, m_sGridGeometry.vNormal);

    if (std::abs(dDenom) < 1e-12)
    {
        return false;
    }

    const double dT = glm::dot(
        m_sGridGeometry.vOrigin - vRayOrigin,
        m_sGridGeometry.vNormal
    ) / dDenom;

    if (dT < 0.0)
    {
        return false;
    }

    vResult = vRayOrigin + vRayDirection * dT;

    return true;
}

void CApplication::SetGridPreset(EGridPreset eNewPreset)
{
    if (m_eCurrentPreset == eNewPreset)
    {
        return;
    }

    m_eCurrentPreset = eNewPreset;
    m_sGridGeometry = CreateGridGeometry(m_eCurrentPreset);

    ApplyCameraViewMode(m_eCameraViewMode);

    std::cout << "Grid preset: " << GetGridPresetName(m_eCurrentPreset) << '\n';
}

void CApplication::ApplyCameraViewMode(ECameraViewMode eMode)
{
    if (m_pCamera == nullptr)
    {
        return;
    }

    const SCameraViewModeSettings sSettings =
        GetCameraViewModeSettings(eMode);

    m_eCameraViewMode = eMode;
    m_bUseOrthographicProjection = sSettings.bUseOrthographicProjection;

    m_dDefaultCameraYawRadians = sSettings.dYawRadians;
    m_dDefaultCameraPitchRadians = sSettings.dPitchRadians;

    m_pCamera->Reset(
        m_sGridGeometry.vOrigin,
        m_dDefaultCameraDistance,
        m_dDefaultCameraYawRadians,
        m_dDefaultCameraPitchRadians
    );

    std::cout << "Camera view: "
        << GetCameraViewModeName(m_eCameraViewMode)
        << '\n';

    std::cout << "Projection: "
        << (m_bUseOrthographicProjection ? "orthographic" : "perspective")
        << '\n';
}

void CApplication::PrintControls() const
{
    std::cout << '\n';
    std::cout << "Controls:\n";
    std::cout << "  Middle mouse button + move         : pan\n";
    std::cout << "  Middle mouse double click          : reset camera to current grid\n";
    std::cout << "  Shift + middle mouse button + move : orbit\n";
    std::cout << "  Left mouse button + move           : orbit fallback\n";
    std::cout << "  Mouse wheel                        : zoom to cursor\n";
    std::cout << "  R                                  : reset current camera view\n";
    std::cout << "  P                                  : toggle orthographic/perspective projection\n";
    std::cout << "  F1                                 : top orthographic view\n";
    std::cout << "  F2                                 : front orthographic view\n";
    std::cout << "  F3                                 : right orthographic view\n";
    std::cout << "  F4                                 : isometric orthographic view\n";
    std::cout << "  F5                                 : perspective view\n";
    std::cout << "  B                                  : toggle infinite/bounded grid\n";
    std::cout << "  M                                  : toggle lines/dots mode\n";
    std::cout << "  C                                  : toggle manual depth clamp\n";
    std::cout << "  F                                  : toggle grid plane fill\n";
    std::cout << "  G                                  : toggle depth zones debug view\n";
    std::cout << "  1                                  : simple grid\n";
    std::cout << "  2                                  : large offset grid\n";
    std::cout << "  3                                  : rotated grid\n";
    std::cout << "  4                                  : large offset + rotated grid\n";
    std::cout << "  Esc                                : exit\n";
    std::cout << '\n';
}

void CApplication::PrintInitialState() const
{
    std::cout << "Camera view: "
        << GetCameraViewModeName(m_eCameraViewMode)
        << '\n';
    std::cout << "Grid preset: " << GetGridPresetName(m_eCurrentPreset) << '\n';
    std::cout << "Projection: "
        << (m_bUseOrthographicProjection ? "orthographic" : "perspective")
        << '\n';
    std::cout << "Grid bounds: "
        << (m_sGridStyle.bIsBounded ? "bounded" : "infinite")
        << '\n';
    std::cout << "Grid mode: "
        << (m_sGridStyle.bDrawDots ? "dots" : "lines")
        << '\n';
    std::cout << "Depth clamp: "
        << (m_sGridStyle.bClampDepth ? "enabled" : "disabled")
        << '\n';
    std::cout << "Grid plane fill: "
        << (m_sGridStyle.bDrawPlane ? "enabled" : "disabled")
        << '\n';
    std::cout << "Depth zones debug: "
        << (m_sGridStyle.bDebugDepthZones ? "enabled" : "disabled")
        << '\n';
}

void CApplication::GlfwErrorCallback(int nErrorCode, const char* pszDescription)
{
    std::cerr << "GLFW error " << nErrorCode << ": " << pszDescription << '\n';
}

void CApplication::MouseButtonCallback(
    GLFWwindow* pWindow,
    int nButton,
    int nAction,
    int nMods
)
{
    (void)nMods;

    CApplication* pApplication =
        static_cast<CApplication*>(glfwGetWindowUserPointer(pWindow));

    if (
        pApplication == nullptr ||
        pApplication->m_pCamera == nullptr
        )
    {
        return;
    }

    COrbitCamera* pCamera = pApplication->m_pCamera.get();

    double dMouseX = 0.0;
    double dMouseY = 0.0;

    glfwGetCursorPos(pWindow, &dMouseX, &dMouseY);

    if (nButton == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        if (nAction == GLFW_PRESS)
        {
            static double s_dLastMiddleClickTime = -1.0;

            const double dCurrentTime = glfwGetTime();
            const double dDoubleClickInterval = 0.35;

            const bool bIsDoubleClick =
                s_dLastMiddleClickTime > 0.0 &&
                (dCurrentTime - s_dLastMiddleClickTime) <= dDoubleClickInterval;

            s_dLastMiddleClickTime = dCurrentTime;

            if (bIsDoubleClick)
            {
                pApplication->ApplyCameraViewMode(
                    pApplication->m_eCameraViewMode
                );

                std::cout << "Camera reset by middle mouse double click\n";
                return;
            }

            const bool bShiftPressed =
                glfwGetKey(pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(pWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

            if (bShiftPressed)
            {
                pCamera->BeginOrbit(dMouseX, dMouseY);
            }
            else
            {
                pCamera->BeginPan(dMouseX, dMouseY);
            }
        }
        else if (nAction == GLFW_RELEASE)
        {
            pCamera->EndDrag();
        }
    }

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

void CApplication::CursorPositionCallback(
    GLFWwindow* pWindow,
    double dMouseX,
    double dMouseY
)
{
    CApplication* pApplication =
        static_cast<CApplication*>(glfwGetWindowUserPointer(pWindow));

    if (
        pApplication == nullptr ||
        pApplication->m_pCamera == nullptr
        )
    {
        return;
    }

    pApplication->m_pCamera->UpdateDrag(dMouseX, dMouseY);
}

void CApplication::ScrollCallback(
    GLFWwindow* pWindow,
    double dOffsetX,
    double dOffsetY
)
{
    (void)dOffsetX;

    CApplication* pApplication =
        static_cast<CApplication*>(glfwGetWindowUserPointer(pWindow));

    if (
        pApplication == nullptr ||
        pApplication->m_pCamera == nullptr
        )
    {
        return;
    }

    COrbitCamera* pCamera = pApplication->m_pCamera.get();

    glm::dvec3 vGridPointBeforeZoom(0.0);

    const bool bHasPointBeforeZoom =
        pApplication->TryGetCursorGridPoint(vGridPointBeforeZoom);

    pCamera->AddZoom(dOffsetY);

    if (!bHasPointBeforeZoom)
    {
        return;
    }

    glm::dvec3 vGridPointAfterZoom(0.0);

    const bool bHasPointAfterZoom =
        pApplication->TryGetCursorGridPoint(vGridPointAfterZoom);

    if (!bHasPointAfterZoom)
    {
        return;
    }

    const glm::dvec3 vTargetCorrection =
        vGridPointBeforeZoom - vGridPointAfterZoom;

    pCamera->SetTarget(pCamera->GetTarget() + vTargetCorrection);
}