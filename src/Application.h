#pragma once

#include "AxisMarkerRenderer.h"
#include "DemoSceneRenderer.h"
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

    // Создаёт ортографическую projection matrix.
    //
    // Перспективная проекция больше не используется.
    glm::dmat4 CreateProjectionMatrix(
        double dAspect,
        double dNearPlane,
        double dFarPlane
    ) const;

    // Возвращает позицию курсора в OpenGL NDC.
    glm::dvec2 GetCursorNdc() const;

    // Находит точку пересечения луча из курсора с текущей плоскостью сетки.
    bool TryGetCursorGridPoint(glm::dvec3& vResult) const;

    // Сбрасывает камеру в дефолтный ортографический вид сверху.
    void ResetCameraToDefaultView();

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

    // Текущий preset сетки.
    //
    // Сейчас оставлен только Simple как дефолтный рабочий режим.
    EGridPreset m_eCurrentPreset;

    // Камера.
    std::unique_ptr<COrbitCamera> m_pCamera;

    // Дефолтное расстояние камеры.
    double m_dDefaultCameraDistance;

    // Дефолтный yaw камеры.
    //
    // Подбирается так, чтобы при запуске:
    // - X смотрела вправо;
    // - Y смотрела вверх.
    double m_dDefaultCameraYawRadians;

    // Дефолтный pitch камеры для вида сверху.
    //
    // Используем почти вертикальный угол, чтобы не получить вырождение basis камеры.
    double m_dDefaultCameraPitchRadians;

    // Состояние клавиши B на прошлом кадре.
    //
    // Нужно, чтобы переключение происходило один раз на нажатие.
    bool m_bWasBPressed;

    // Состояние клавиши M на прошлом кадре.
    //
    // Нужно, чтобы переключение происходило один раз на нажатие.
    bool m_bWasMPressed;

    // Состояние клавиши G на прошлом кадре.
    //
    // Нужно, чтобы переключение depth debug происходило один раз на нажатие.
    bool m_bWasGPressed;

    // Отображать ли тестовые модельные объекты.
    //
    // Эти объекты не являются частью сетки.
    // Они нужны только для проверки того, как аналитическая сетка
    // взаимодействует с обычной сценой через depth buffer.
    bool m_bShowDemoObjects;

    // Состояние клавиши O на прошлом кадре.
    //
    // Нужно, чтобы переключение происходило один раз на нажатие.
    bool m_bWasOPressed;

    // Рисовать ли сетку поверх всех объектов.
    //
    // false:
    //   обычный режим. Сетка участвует в depth test и корректно
    //   взаимодействует с объектами сцены.
    //
    // true:
    //   x-ray режим. Сетка рисуется поверх фигур и визуально
    //   проходит через них.
    bool m_bGridXrayMode;

    // Состояние клавиши X на прошлом кадре.
    //
    // Нужно, чтобы переключение x-ray режима происходило один раз на нажатие.
    bool m_bWasXPressed;
};