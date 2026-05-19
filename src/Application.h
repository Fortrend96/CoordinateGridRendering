#pragma once

#include "AxisMarkerRenderer.h"
#include "DemoSceneRenderer.h"
#include "GridRenderer.h"
#include "OrbitCamera.h"
#include "ShaderProgram.h"
#include "ViewUniformBuffer.h"

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

	// Уменьшает/увеличивает шаг сетки по X.
	//
	// При ручном изменении шага adaptive grid автоматически выключается.
	void MultiplyGridStepX(double dMultiplier);

	// Уменьшает/увеличивает шаг сетки по Y.
	//
	// При ручном изменении шага adaptive grid автоматически выключается.
	void MultiplyGridStepY(double dMultiplier);

	// Меняет частоту major-линий.
	void ChangeMajorLineFrequency(int nDelta);

	// Печатает текущие параметры шага сетки.
	void PrintGridSpacingState() const;

	// Печатает список доступного управления.
	void PrintControls() const;

	// Печатает начальное состояние приложения.
	void PrintInitialState() const;

private:
	// GLFW error callback.
	static void GlfwErrorCallback(
		int nErrorCode,
		const char* pszDescription
	);

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

	// Рендерер аналитической координатной сетки.
	CGridRenderer m_gridRenderer;

	// Рендерер маркера начала координат.
	CAxisMarkerRenderer m_axisMarkerRenderer;

	// Рендерер тестовых объектов сцены.
	CDemoSceneRenderer m_demoSceneRenderer;

	// Текущий стиль сетки.
	SGridStyle m_sGridStyle;

	// Текущая геометрия сетки.
	SGridGeometry m_sGridGeometry;

	// Orbit-камера.
	std::unique_ptr<COrbitCamera> m_pCamera;

	// Начальная дистанция камеры.
	double m_dDefaultCameraDistance;

	// Начальный yaw камеры.
	//
	// Подобран так, чтобы при запуске:
	// - ось X была направлена вправо;
	// - ось Y была направлена вверх.
	double m_dDefaultCameraYawRadians;

	// Начальный pitch камеры для вида сверху.
	//
	// Используется почти вертикальный угол, чтобы не получить вырождение
	// базиса камеры.
	double m_dDefaultCameraPitchRadians;

	// Запрашиваемое количество sample'ов для MSAA.
	int m_nRequestedMsaaSamples;

	// Фактическое количество sample'ов, которое выдал OpenGL context.
	int m_nActualMsaaSamples;

	// Активен ли MSAA по факту.
	bool m_bIsMsaaActive;

	// Включать ли sample shading для процедурной сетки.
	//
	// По умолчанию выключено, чтобы сначала проверять обычный аппаратный MSAA.
	bool m_bUseSampleShadingForGrid;

	// Состояния клавиш на прошлом кадре.
	//
	// Нужны, чтобы переключатели срабатывали один раз на нажатие,
	// а не каждый кадр при удержании клавиши.
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
	//
	// Эти объекты нужны для проверки взаимодействия сетки с depth buffer.
	bool m_bShowDemoObjects;

	// Режим отображения сетки поверх объектов.
	//
	// false:
	//   сетка участвует в depth test и корректно перекрывается объектами.
	//
	// true:
	//   depth test для сетки отключается, поэтому сетка видна поверх фигур.
	bool m_bGridXrayMode;

	// Общий uniform buffer данных текущего viewport/camera.
	CViewUniformBuffer m_viewUniformBuffer;
};