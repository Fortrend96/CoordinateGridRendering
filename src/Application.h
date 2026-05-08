#pragma once

#include "AxisMarkerRenderer.h"
#include "CameraViewMode.h"
#include "GridPresets.h"
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
// - загрузку shader'ов;
// - создание renderer'ов;
// - обработку input;
// - camera controls;
// - render loop.
//
// main.cpp после рефакторинга только создаёт CApplication и вызывает Run().
class CApplication
{
public:
    // Создаёт объект приложения.
    CApplication();

    // Освобождает ресурсы приложения.
    ~CApplication();

    // Копирование запрещено, потому что приложение владеет GLFWwindow и OpenGL-ресурсами.
    CApplication(const CApplication&) = delete;

    // Копирующее присваивание запрещено, потому что приложение владеет GLFWwindow и OpenGL-ресурсами.
    CApplication& operator=(const CApplication&) = delete;

    // Запускает приложение.
    //
    // Возвращает:
    // - 0 при нормальном завершении;
    // - исключение при ошибке инициализации.
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
    // Рассчитывает near/far planes для текущего положения камеры.
    void CalculateCameraClippingPlanes(
        double& dNearPlane,
        double& dFarPlane
    ) const;

    // Создаёт projection matrix для текущего режима.
    glm::dmat4 CreateProjectionMatrix(
        double dAspect,
        double dNearPlane,
        double dFarPlane
    ) const;

    // Возвращает позицию курсора в OpenGL NDC.
    glm::dvec2 GetCursorNdc() const;

    // Находит точку пересечения луча из курсора с текущей плоскостью сетки.
    bool TryGetCursorGridPoint(glm::dvec3& vResult) const;

    // Переключает preset сетки.
    void SetGridPreset(EGridPreset eNewPreset);

    // Применяет выбранный режим камеры:
    // - задаёт тип проекции;
    // - задаёт yaw/pitch;
    // - сбрасывает камеру на origin текущей сетки.
    void ApplyCameraViewMode(ECameraViewMode eMode);

    // Печатает управление в консоль.
    void PrintControls() const;

    // Печатает текущее состояние демо.
    void PrintInitialState() const;

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
    //
    // Маркер оставляем как отдельную вспомогательную сущность демо,
    // не как часть логики сетки.
    CShaderProgram m_axisMarkerShaderProgram;

    // Renderer аналитической сетки.
    CGridRenderer m_gridRenderer;

    // Renderer маркера начала координат.
    CAxisMarkerRenderer m_axisMarkerRenderer;

    // Текущий стиль сетки.
    SGridStyle m_sGridStyle;

    // Текущая геометрия сетки.
    SGridGeometry m_sGridGeometry;

    // Текущий preset сетки.
    EGridPreset m_eCurrentPreset;

    // Текущий режим вида камеры.
    ECameraViewMode m_eCameraViewMode;

    // Камера.
    std::unique_ptr<COrbitCamera> m_pCamera;

    // Использовать ли ортографическую проекцию.
    bool m_bUseOrthographicProjection;

    // Дефолтное расстояние камеры.
    double m_dDefaultCameraDistance;

    // Дефолтный yaw камеры.
    double m_dDefaultCameraYawRadians;

    // Дефолтный pitch камеры.
    double m_dDefaultCameraPitchRadians;

    // Состояние клавиши P на прошлом кадре.
    bool m_bWasPPressed;

    // Состояние клавиши B на прошлом кадре.
    bool m_bWasBPressed;

    // Состояние клавиши M на прошлом кадре.
    bool m_bWasMPressed;

    // Состояние клавиши C на прошлом кадре.
    bool m_bWasCPressed;

    // Состояние клавиши F на прошлом кадре.
    bool m_bWasFPressed;

    // Состояние клавиши G на прошлом кадре.
    bool m_bWasGPressed;

    // Состояние клавиши F1 на прошлом кадре.
    bool m_bWasF1Pressed;

    // Состояние клавиши F2 на прошлом кадре.
    bool m_bWasF2Pressed;

    // Состояние клавиши F3 на прошлом кадре.
    bool m_bWasF3Pressed;

    // Состояние клавиши F4 на прошлом кадре.
    bool m_bWasF4Pressed;

    // Состояние клавиши F5 на прошлом кадре.
    bool m_bWasF5Pressed;
};