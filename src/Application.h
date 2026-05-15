#pragma once

#include "AxisMarkerRenderer.h"
#include "DemoSceneRenderer.h"
#include "GridRenderer.h"
#include "OrbitCamera.h"
#include "ShaderProgram.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <memory>

// Главный класс демо-приложения.
//
// Отвечает за:
// - создание окна;
// - инициализацию OpenGL;
// - загрузку shader-программ;
// - обработку пользовательского ввода;
// - управление камерой;
// - отрисовку тестовой сцены, координатной сетки и маркера origin.
class CApplication
{
public:
    // Создаёт объект приложения с дефолтными параметрами.
    CApplication();

    // Освобождает ресурсы приложения.
    ~CApplication();

    // Копирование запрещено, потому что класс владеет GLFWwindow
    // и OpenGL-ресурсами.
    CApplication(const CApplication&) = delete;
    CApplication& operator=(const CApplication&) = delete;

    // Запускает приложение.
    int Run();

private:
    // Инициализирует GLFW.
    void InitializeGlfw();

    // Создаёт окно и OpenGL context.
    void CreateWindow();

    // Инициализирует GLAD и базовые OpenGL-состояния.
    void InitializeOpenGl();

    // Загружает shader-программы.
    void LoadShaders();

    // Инициализирует рендереры, камеру и начальную геометрию сетки.
    void InitializeScene();

    // Регистрирует GLFW callback-и.
    void RegisterCallbacks();

    // Главный цикл приложения.
    void MainLoop();

    // Обрабатывает клавиатурный ввод текущего кадра.
    void ProcessInput();

    // Рисует один кадр.
    void RenderFrame();

    // Освобождает GLFW/OpenGL-ресурсы.
    void Shutdown();

private:
    // Создаёт стандартную координатную плоскость XOY.
    SGridGeometry CreateDefaultGridGeometry() const;

    // Подбирает near/far planes под текущий zoom камеры.
    void CalculateCameraClippingPlanes(
        double& dNearPlane,
        double& dFarPlane
    ) const;

    // Создаёт ортографическую projection matrix.
    glm::dmat4 CreateProjectionMatrix(
        double dAspect,
        double dNearPlane,
        double dFarPlane
    ) const;

    // Возвращает позицию курсора в NDC-координатах OpenGL.
    glm::dvec2 GetCursorNdc() const;

    // Находит точку пересечения луча из курсора с плоскостью сетки.
    bool TryGetCursorGridPoint(glm::dvec3& vResult) const;

    // Сбрасывает камеру в начальный вид сверху.
    void ResetCameraToDefaultView();


    // Включает/выключает adaptive grid.
    void ToggleAdaptiveGrid();

    // Ручное изменение шага сетки.
    // При ручном изменении adaptive grid автоматически выключается.
    void MultiplyGridStepX(double dMultiplier);
    void MultiplyGridStepY(double dMultiplier);

    // Меняет частоту крупных линий.
    void ChangeMajorLineFrequency(int nDelta);

    // Печатает текущие параметры шага сетки.
    void PrintGridSpacingState() const;


    // Печатает список доступного управления.
    void PrintControls() const;

    // Печатает начальное состояние приложения.
    void PrintInitialState() const;

private:
    // GLFW error callback.
    static void GlfwErrorCallback(int nErrorCode, const char* pszDescription);

    // GLFW mouse button callback.
    static void MouseButtonCallback(
        GLFWwindow* pWindow,
        int nButton,
        int nAction,
        int nMods
    );

    // GLFW cursor position callback.
    static void CursorPositionCallback(
        GLFWwindow* pWindow,
        double dMouseX,
        double dMouseY
    );

    // GLFW scroll callback.
    static void ScrollCallback(
        GLFWwindow* pWindow,
        double dOffsetX,
        double dOffsetY
    );

private:
    // Окно GLFW.
    GLFWwindow* m_pWindow;

    // Начальный размер окна.
    int m_nInitialWindowWidth;
    int m_nInitialWindowHeight;

    // Shader-программа координатной сетки.
    CShaderProgram m_gridShaderProgram;

    // Shader-программа маркера начала координат.
    CShaderProgram m_axisMarkerShaderProgram;

    // Shader-программа тестовых модельных объектов.
    CShaderProgram m_sceneObjectShaderProgram;

    // Рендереры сцены.
    CGridRenderer m_gridRenderer;
    CAxisMarkerRenderer m_axisMarkerRenderer;
    CDemoSceneRenderer m_demoSceneRenderer;

    // Текущие параметры сетки.
    SGridStyle m_sGridStyle;
    SGridGeometry m_sGridGeometry;

    // Orbit-камера.
    std::unique_ptr<COrbitCamera> m_pCamera;

    // Начальные параметры камеры.
    double m_dDefaultCameraDistance;
    double m_dDefaultCameraYawRadians;
    double m_dDefaultCameraPitchRadians;

    // Состояния клавиш на прошлом кадре.
    //
    // Нужны, чтобы переключатели срабатывали один раз на нажатие.
    bool m_bWasBPressed;
    bool m_bWasMPressed;
    bool m_bWasFPressed;
    bool m_bWasGPressed;
    bool m_bWasAPressed;
    bool m_bWasOPressed;
    bool m_bWasXPressed;

    bool m_bWasLeftBracketPressed;
    bool m_bWasRightBracketPressed;
    bool m_bWasSemicolonPressed;
    bool m_bWasApostrophePressed;
    bool m_bWasCommaPressed;
    bool m_bWasPeriodPressed;

    // Показывать ли тестовые модельные объекты.
    bool m_bShowDemoObjects;

    // false — сетка участвует в depth test.
    // true  — сетка рисуется поверх объектов.
    bool m_bGridXrayMode;
};