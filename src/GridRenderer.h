#pragma once

#include "ShaderProgram.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

// Геометрия координатной сетки в world-space.
//
// Это исходное описание плоскости сетки.
// Сетка не хранит готовые линии. Линии вычисляются аналитически в shader'е.
struct SGridGeometry
{
	// Начало координат сетки в world-space.
	glm::dvec3 vOrigin;

	// Локальная ось X сетки в world-space.
	glm::dvec3 vAxisX;

	// Локальная ось Y сетки в world-space.
	glm::dvec3 vAxisY;

	// Нормаль плоскости сетки в world-space.
	glm::dvec3 vNormal;
};

// Данные текущего viewport/camera.
//
// Эти данные не относятся конкретно к сетке.
// В будущем такую структуру логично вынести в общий uniform buffer,
// который смогут использовать все shader'ы сцены.
struct SViewData
{
	// View matrix текущей камеры.
	glm::dmat4 mView;

	// Projection matrix текущей камеры.
	glm::dmat4 mProjection;

	// Обратная projection matrix.
	//
	// Используется в shader'е для восстановления eye-space луча пикселя.
	glm::dmat4 mInvProjection;

	// Projection * View.
	//
	// Используется на C++ стороне для оценки экранного размера шага сетки.
	glm::dmat4 mViewProj;

	// Размер viewport в пикселях.
	glm::dvec2 vViewportSize;

	// Признак ортографической проекции.
	//
	// Сейчас проект использует только orthographic projection,
	// но поле оставлено как часть общих view-данных.
	bool bIsOrthographicProjection;
};

// Входные настройки внешнего вида сетки.
//
// Это именно настройки, а не готовые данные для shader'а.
// На основе этих параметров и текущего SViewData вычисляется SGridShaderData.
struct SGridStyle
{
	// Малый шаг сетки по локальной оси X.
	double dMinorStepX;

	// Малый шаг сетки по локальной оси Y.
	double dMinorStepY;

	// Количество малых шагов в одном большом шаге.
	//
	// Большой шаг отдельно не хранится, чтобы не было противоречий.
	int nMajorLineFrequency;

	// Толщина minor-линий в пикселях.
	float fMinorThickness;

	// Толщина major-линий в пикселях.
	float fMajorThickness;

	// Толщина осей X/Y в пикселях.
	float fAxisThickness;

	// Ширина ручного shader-AA в пикселях.
	//
	// 0.0 — ручное shader-сглаживание выключено.
	// Основной способ сглаживания сейчас — аппаратный MSAA.
	float fShaderAntialiasWidth;

	// Минимально допустимое значение abs(dot(rayDirection, gridNormal)).
	//
	// Если луч почти параллелен плоскости сетки, пересечение становится
	// плохо обусловленным, и сетку лучше не рисовать.
	double dMinViewNormalDot;

	// Безопасный отступ от depth 0 и 1.
	//
	// Используется для ручного depth clamp, чтобы не писать ровно 0 или 1.
	double dSafeDepthEpsilon;

	// Включить debug-режим зон глубины.
	//
	// Красный  — ближе near plane.
	// Зелёный — нормальный диапазон depth.
	// Синий   — дальше far plane.
	bool bDebugDepthZones;

	// Рисовать ли полупрозрачную заливку плоскости сетки.
	bool bDrawPlane;

	// Ограниченная или бесконечная сетка.
	bool bIsBounded;

	// Границы ограниченной сетки в локальных координатах:
	// xMin, yMin, xMax, yMax.
	glm::dvec4 vBounds;

	// Рисовать сетку точками вместо линий.
	bool bDrawDots;

	// Радиус точек в пикселях.
	float fDotRadius;

	// Использовать ли автоматический подбор шага сетки.
	//
	// Сам расчёт adaptive step выполняется на C++ стороне.
	// Shader получает уже готовые uMinorStep/uMajorStep.
	bool bUseAdaptiveStep;

	// Желаемый экранный размер minor step в пикселях.
	//
	// Используется только на C++ стороне при bUseAdaptiveStep == true.
	double dTargetMinorStepPixels;

	// Цвет заливки верхней стороны плоскости.
	glm::vec4 vPlaneColorTop;

	// Цвет minor-линий верхней стороны.
	glm::vec4 vMinorColorTop;

	// Цвет major-линий верхней стороны.
	glm::vec4 vMajorColorTop;

	// Цвет положительного направления оси X сверху.
	glm::vec4 vXAxisColorTop;

	// Цвет положительного направления оси Y сверху.
	glm::vec4 vYAxisColorTop;

	// Цвет заливки нижней стороны плоскости.
	glm::vec4 vPlaneColorBottom;

	// Цвет minor-линий нижней стороны.
	glm::vec4 vMinorColorBottom;

	// Цвет major-линий нижней стороны.
	glm::vec4 vMajorColorBottom;

	// Цвет положительного направления оси X снизу.
	glm::vec4 vXAxisColorBottom;

	// Цвет положительного направления оси Y снизу.
	glm::vec4 vYAxisColorBottom;
};

// Согласованные шаги сетки.
//
// Эти значения вычисляются на C++ стороне и передаются в shader готовыми.
// Shader не выбирает шаг самостоятельно.
struct SGridComputedSteps
{
	// Готовый minor step по X/Y.
	glm::dvec2 vMinorStep;

	// Готовый major step по X/Y.
	glm::dvec2 vMajorStep;
};

// Данные сетки, подготовленные для shader'а.
//
// Это результат вычисления на основе:
// - SViewData;
// - SGridGeometry;
// - SGridStyle.
//
// Render() не должен заново считать эти параметры.
// Он только отправляет их в shader и вызывает draw.
struct SGridShaderData
{
	// Начало координат сетки в eye-space.
	glm::dvec3 vGridOriginEye;

	// Ось X сетки в eye-space.
	glm::dvec3 vGridAxisXEye;

	// Ось Y сетки в eye-space.
	glm::dvec3 vGridAxisYEye;

	// Нормаль плоскости сетки в eye-space.
	glm::dvec3 vGridNormalEye;

	// Anchor pattern'а сетки в eye-space.
	//
	// Используется, чтобы строить линии не от огромных абсолютных координат,
	// а от локальной точки рядом с видимой областью.
	glm::dvec3 vGridPatternOriginEye;

	// Готовый minor step по X/Y.
	glm::dvec2 vMinorStep;

	// Готовый major step по X/Y.
	glm::dvec2 vMajorStep;

	// Минимально допустимый угол между лучом и плоскостью сетки.
	double dMinViewNormalDot;

	// Отступ для ручного depth clamp.
	double dSafeDepthEpsilon;

	// Включён ли debug-режим зон глубины.
	bool bDebugDepthZones;

	// Рисовать ли заливку плоскости.
	bool bDrawPlane;

	// Ограниченная или бесконечная сетка.
	bool bIsBounded;

	// Границы ограниченной сетки:
	// xMin, yMin, xMax, yMax.
	glm::dvec4 vBounds;

	// Рисовать ли сетку точками.
	bool bDrawDots;

	// Показывать ли minor-сетку.
	bool bShowMinorGrid;

	// Показывать ли major-сетку.
	bool bShowMajorGrid;

	// Показывать ли оси X/Y.
	bool bShowAxes;

	// Толщина minor-линий в пикселях.
	float fMinorThickness;

	// Толщина major-линий в пикселях.
	float fMajorThickness;

	// Толщина осей в пикселях.
	float fAxisThickness;

	// Радиус точек в пикселях.
	float fDotRadius;

	// Ширина ручного shader-AA.
	//
	// 0.0 — shader-AA выключен.
	float fShaderAntialiasWidth;

	// Цвет заливки верхней стороны плоскости.
	glm::vec4 vPlaneColorTop;

	// Цвет minor-линий верхней стороны.
	glm::vec4 vMinorColorTop;

	// Цвет major-линий верхней стороны.
	glm::vec4 vMajorColorTop;

	// Цвет оси X сверху.
	glm::vec4 vXAxisColorTop;

	// Цвет оси Y сверху.
	glm::vec4 vYAxisColorTop;

	// Цвет заливки нижней стороны плоскости.
	glm::vec4 vPlaneColorBottom;

	// Цвет minor-линий нижней стороны.
	glm::vec4 vMinorColorBottom;

	// Цвет major-линий нижней стороны.
	glm::vec4 vMajorColorBottom;

	// Цвет оси X снизу.
	glm::vec4 vXAxisColorBottom;

	// Цвет оси Y снизу.
	glm::vec4 vYAxisColorBottom;
};

// Рендерер аналитической координатной сетки.
//
// Сетка рисуется fullscreen quad'ом.
// Основная логика построения линий находится во fragment shader'е.
//
// Архитектурно класс разделён на два этапа:
// 1. ComputeGridData() — подготовить данные для shader'а.
// 2. Render()          — отправить готовые данные и выполнить draw call.
class CGridRenderer
{
public:
	// Создаёт renderer с дефолтной геометрией и стилем сетки.
	CGridRenderer();

	// Освобождает OpenGL-ресурсы renderer'а.
	~CGridRenderer();

	// Копирование запрещено, потому что renderer владеет OpenGL VAO.
	CGridRenderer(const CGridRenderer&) = delete;
	CGridRenderer& operator=(const CGridRenderer&) = delete;

	// Перемещение разрешено.
	CGridRenderer(CGridRenderer&& other) noexcept;
	CGridRenderer& operator=(CGridRenderer&& other) noexcept;

	// Создаёт OpenGL-ресурсы.
	void Initialize();

	// Освобождает OpenGL-ресурсы.
	void Destroy();

	// Задаёт геометрию плоскости сетки.
	void SetGeometry(const SGridGeometry& sGeometry);

	// Задаёт стиль отображения сетки.
	void SetStyle(const SGridStyle& sStyle);

	// Возвращает текущую геометрию сетки.
	const SGridGeometry& GetGeometry() const;

	// Возвращает текущий стиль сетки.
	const SGridStyle& GetStyle() const;

	// Готовит данные сетки для текущего вида.
	//
	// Здесь выполняется вся смысловая подготовка:
	// - расчёт adaptive step;
	// - вычисление major step;
	// - перевод плоскости сетки в eye-space;
	// - вычисление anchor рядом с видимой областью;
	// - вычисление LOD-флагов minor/major/axes.
	//
	// Возвращает false, если сетку в текущем состоянии нельзя рисовать.
	bool ComputeGridData(
		SGridShaderData& sGridData,
		const SViewData& sViewData
	) const;

	// Рисует сетку по заранее подготовленным данным.
	//
	// Метод не должен пересчитывать параметры сетки.
	// Он только передаёт uniform'ы и рисует fullscreen quad.
	void Render(
		const CShaderProgram& shaderProgram,
		const SViewData& sViewData,
		const SGridShaderData& sGridData
	) const;

private:
	// Вычисляет согласованные minor/major шаги сетки.
	//
	// Если adaptive step включён, minor step пересчитывается по текущему виду.
	// Major step всегда получается как minorStep * majorLineFrequency.
	SGridComputedSteps ComputeGridSteps(
		const SViewData& sViewData,
		bool& bShowMinorGrid,
		bool& bShowMajorGrid,
		bool& bShowAxes
	) const;

private:
	// VAO для fullscreen quad.
	//
	// Вершины quad'а генерируются во vertex shader'е через gl_VertexID.
	GLuint m_nFullscreenVao;

	// Текущая геометрия сетки.
	SGridGeometry m_sGeometry;

	// Текущий стиль сетки.
	SGridStyle m_sStyle;
};