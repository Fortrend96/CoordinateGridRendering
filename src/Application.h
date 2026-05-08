#pragma once

#include "AxisMarkerRenderer.h"
#include "DemoSceneRenderer.h"
#include "GridRenderer.h"

#include "ShaderProgram.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <memory>

class COrbitCamera;

// Главный класс приложения.
// Отвечает за:
// - создание окна;
// - инициализацию OpenGL;
// - загрузку shader'ов;
// - создание renderer'ов;
// - обработку input;
// - camera controls;
// - render loop.
class CApplication
{
public:
    // Создаёт объект приложения.
    CApplication();

    // Освобождает ресурсы приложения.
    ~CApplication();

    // Копирование и копирующее присваивание запрещено запрещено, потому что приложение владеет GLFWwindow и OpenGL-ресурсами.
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

    // Загружает shader programs.
    void LoadShaders();

    // Создаёт renderer'ы, камеру и начальное состояние сцены.
    void InitializeScene();

    // Устанавливает GLFW callback'и.
    void RegisterCallbacks();

    // Главный цикл приложения.
    void MainLoop();

    // Обрабатывает input текущего кадра.
    void ProcessInput();

    // Рисует один кадр.
    void RenderFrame();

    // Освобождает ресурсы.
    void Shutdown();

private:
    // Рассчитывает ближнюю/дальнюю плоскости для текущего положения камеры.
    void CalculateCameraClippingPlanes(
        double& dNearPlane,
        double& dFarPlane
    ) const;

    // Создаёт ортографическую матрицу проекции.
    glm::dmat4 CreateProjectionMatrix(
        double dAspect,
        double dNearPlane,
        double dFarPlane
    ) const;

    // Возвращает позицию курсора в OpenGL NDC (нормализованные координаты устройства).
    glm::dvec2 GetCursorNdc() const;

    // Находит точку пересечения луча из курсора с текущей плоскостью сетки.
    bool TryGetCursorGridPoint(glm::dvec3& vResult) const;

    // Сбрасывает камеру в дефолтный ортографический вид сверху.
    void ResetCameraToDefaultView();

    // Печатает управление в консоль.
    void PrintControls() const;

    // Печатает текущее состояние демо.
    void PrintInitialState() const;

    // Задаем геометрию по умолчанию
    SGridGeometry CreateDefaultGridGeometry() const;

private:
    // GLFW error callback.
    static void GlfwErrorCallback(int nErrorCode, const char* pszDescription);

    // GLFW mouse button callback.
    static void MouseButtonCallback(GLFWwindow* pWindow, int nButton, int nAction, int nMods);

    // GLFW cursor position callback.
    static void CursorPositionCallback(GLFWwindow* pWindow, double dMouseX, double dMouseY);

    // GLFW scroll callback.
    static void ScrollCallback(GLFWwindow* pWindow, double dOffsetX, double dOffsetY);

private:
    // Указатель на окно GLFW.
    GLFWwindow* m_pWindow;

    // Ширина окна при запуске.
    int m_nInitialWindowWidth;

    // Высота окна при запуске.
    int m_nInitialWindowHeight;

    // Shader program для сетки.
    CShaderProgram m_gridShaderProgram;

    // Shader program для маркера начала координат.
    CShaderProgram m_axisMarkerShaderProgram;

    // Shader program для модельных объектов тестовой сцены.
    CShaderProgram m_sceneObjectShaderProgram;

    // Renderer аналитической сетки.
    CGridRenderer m_gridRenderer;

    // Renderer маркера начала координат.
    CAxisMarkerRenderer m_axisMarkerRenderer;

    // Renderer тестовых модельных объектов.
    CDemoSceneRenderer m_demoSceneRenderer;

    // Текущий стиль сетки.
    SGridStyle m_sGridStyle;

    // Текущая геометрия сетки.
    SGridGeometry m_sGridGeometry;

    // Камера.
    std::unique_ptr<COrbitCamera> m_pCamera;

    // Дефолтное расстояние камеры.
    double m_dDefaultCameraDistance;

    // Дефолтный yaw камеры.
    double m_dDefaultCameraYawRadians;

    // Дефолтный pitch камеры для вида сверху.
    double m_dDefaultCameraPitchRadians;

    // Состояние клавиши B на прошлом кадре.
    bool m_bWasBPressed;

    // Состояние клавиши M на прошлом кадре.
    bool m_bWasMPressed;

    // Состояние клавиши G на прошлом кадре.
    bool m_bWasGPressed;

    // Отображать ли тестовые модельные объекты.
    // Эти объекты не являются частью сетки.
    bool m_bShowDemoObjects;

    // Состояние клавиши O на прошлом кадре.
    bool m_bWasOPressed;

    // Рисовать ли сетку поверх всех объектов.
    bool m_bGridXrayMode;

    // Состояние клавиши X на прошлом кадре.
    bool m_bWasXPressed;
};