#pragma once

#include "ShaderProgram.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

// Геометрия координатной сетки в world-space.
//
// Сетка задаётся не набором линий, а плоскостью:
// origin + axisX * x + axisY * y.
struct SGridGeometry
{
    // Начало координат сетки.
    glm::dvec3 vOrigin;

    // Локальная ось X сетки в world-space.
    glm::dvec3 vAxisX;

    // Локальная ось Y сетки в world-space.
    glm::dvec3 vAxisY;

    // Нормаль плоскости сетки.
    glm::dvec3 vNormal;
};

// Данные текущего кадра, необходимые для отрисовки сетки.
struct SGridFrameData
{
    // View matrix текущей камеры.
    glm::dmat4 mView;

    // Projection matrix текущей камеры.
    glm::dmat4 mProjection;

    // Inverse projection matrix.
    //
    // Во fragment shader используется для восстановления eye-space луча
    // текущего пикселя из gl_FragCoord.
    glm::dmat4 mInvProjection;

    // Projection * View.
    //
    // Используется на CPU для подбора adaptive step.
    glm::dmat4 mViewProj;

    // Размер viewport в пикселях.
    glm::dvec2 vViewportSize;

    // Сейчас используется только orthographic projection.
    bool bIsOrthographicProjection;
};

// Настройки внешнего вида сетки.
struct SGridStyle
{
    // Малый шаг сетки в локальных единицах сетки.
    double dMinorStep;

    // Количество малых шагов в одном большом шаге.
    int nMajorLineFrequency;

    // Толщина линий в пикселях.
    float fMinorThickness;
    float fMajorThickness;
    float fAxisThickness;

    // Минимальный угол между лучом камеры и плоскостью сетки.
    //
    // Если камера смотрит почти вдоль плоскости, сетку лучше скрыть,
    // чтобы не показывать сильно искажённые фрагменты.
    double dMinViewNormalDot;

    // Небольшой отступ от 0 и 1 для ручного depth clamp.
    double dSafeDepthEpsilon;

    // Debug-режим зон глубины.
    bool bDebugDepthZones;

    // Рисовать ли полупрозрачную заливку плоскости сетки.
    bool bDrawPlane;

    // Ограниченная или бесконечная сетка.
    bool bIsBounded;

    // Границы ограниченной сетки:
    // xMin, yMin, xMax, yMax.
    glm::dvec4 vBounds;

    // Режим точек вместо линий.
    bool bDrawDots;
    float fDotRadius;

    // Автоматически подбирать шаг сетки в зависимости от zoom.
    bool bUseAdaptiveStep;

    // Желаемый экранный шаг minor-сетки в пикселях.
    double dTargetMinorStepPixels;

    // Внутренние LOD-флаги отображения уровней сетки.
    bool bShowMinorGrid;
    bool bShowMajorGrid;
    bool bShowAxes;

    // Цвета верхней стороны плоскости сетки.
    glm::vec4 vPlaneColorTop;
    glm::vec4 vMinorColorTop;
    glm::vec4 vMajorColorTop;
    glm::vec4 vXAxisColorTop;
    glm::vec4 vYAxisColorTop;

    // Цвета нижней стороны плоскости сетки.
    glm::vec4 vPlaneColorBottom;
    glm::vec4 vMinorColorBottom;
    glm::vec4 vMajorColorBottom;
    glm::vec4 vXAxisColorBottom;
    glm::vec4 vYAxisColorBottom;
};

// Рендерер аналитической координатной сетки.
//
// Сетка рисуется fullscreen quad'ом.
// Вся основная логика находится во fragment shader:
// - восстановление eye-space луча текущего пикселя;
// - пересечение с eye-space плоскостью сетки;
// - вычисление линий/точек;
// - расчёт gl_FragDepth.
class CGridRenderer
{
public:
    CGridRenderer();
    ~CGridRenderer();

    // Копирование запрещено: класс владеет OpenGL VAO.
    CGridRenderer(const CGridRenderer&) = delete;
    CGridRenderer& operator=(const CGridRenderer&) = delete;

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

    const SGridGeometry& GetGeometry() const;
    const SGridStyle& GetStyle() const;

    // Пересчитывает adaptive step под текущий zoom.
    void UpdateAdaptiveStep(const SGridFrameData& sFrameData);

    // Рисует сетку.
    void Render(
        const CShaderProgram& shaderProgram,
        const SGridFrameData& sFrameData
    ) const;

private:
    // VAO для fullscreen quad.
    GLuint m_nFullscreenVao;

    // Текущая геометрия сетки.
    SGridGeometry m_sGeometry;

    // Текущий стиль сетки.
    SGridStyle m_sStyle;
};