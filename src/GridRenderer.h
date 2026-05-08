#pragma once

#include "ShaderProgram.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

// Геометрические параметры координатной сетки.
struct SGridGeometry
{
    // Начало координат сетки в world-space.
    glm::dvec3 vOrigin;

    // Направление локальной оси X сетки в world-space.
    glm::dvec3 vAxisX;

    // Направление локальной оси Y сетки в world-space.
    glm::dvec3 vAxisY;

    // Нормаль к плоскости сетки в world-space.
    glm::dvec3 vNormal;
};

// Визуальные и логические параметры сетки.
// Эта структура описывает не только цвета и толщины,
// но и runtime-режимы отображения:
struct SGridStyle
{
    // Текущий малый шаг сетки в локальных единицах сетки.
    // При включённом adaptive step значение пересчитывается каждый кадр.
    double dMinorStep;

    // Текущий большой шаг сетки в локальных единицах сетки.
    // Обычно равен dMinorStep * nMajorLineFrequency.
    double dMajorStep;

    // Толщина малых линий сетки в условных экранных единицах.
    float fMinorThickness;

    // Толщина больших линий сетки в условных экранных единицах.
    float fMajorThickness;

    // Толщина осей X/Y в условных экранных единицах.
    float fAxisThickness;

    // Минимально допустимый abs(dot(rayDirection, gridNormal)).
    // Если значение меньше, значит камера смотрит почти вдоль плоскости сетки. В таком случае шейдер её скрывает.
    double dMinViewNormalDot;

    // Безопасный отступ от 0 и 1 в depth buffer.
    double dSafeDepthEpsilon;

    // Отладочный режим раскраски зон глубины.
    bool bDebugDepthZones;

    // Ограничивать ли сетку прямоугольником в её локальной системе координат.
    bool bIsBounded;

    // Границы ограниченной сетки в локальной системе координат:
    // x = minX,
    // y = minY,
    // z = maxX,
    // w = maxY.
    glm::dvec4 vBounds;

    // Режим отображения узлов сетки.
    bool bDrawDots;

    // Радиус точки в режиме отображения dots.
    float fDotRadius;

    // Использовать ли автоматический выбор шага сетки при изменении zoom.
    bool bUseAdaptiveStep;

    // Желаемое расстояние между малыми линиями сетки в пикселях.
    // На основе этого значения renderer выбирает красивый шаг 1/2/5 * 10^n.
    double dTargetMinorStepPixels;

    // Количество малых шагов в одном большом шаге.
    int nMajorLineFrequency;

    // Показывать ли малую сетку.
    bool bShowMinorGrid;

    // Показывать ли большую сетку.
    bool bShowMajorGrid;

    // Показывать ли оси X/Y.
    bool bShowAxes;

    // Цвет малых линий при взгляде на верхнюю сторону сетки.
    glm::vec4 vMinorColorTop;

    // Цвет больших линий при взгляде на верхнюю сторону сетки.
    glm::vec4 vMajorColorTop;

    // Цвет оси X при взгляде на верхнюю сторону сетки.
    glm::vec4 vXAxisColorTop;

    // Цвет оси Y при взгляде на верхнюю сторону сетки.
    glm::vec4 vYAxisColorTop;

    // Цвет малых линий при взгляде на нижнюю сторону сетки.
    glm::vec4 vMinorColorBottom;

    // Цвет больших линий при взгляде на нижнюю сторону сетки.
    glm::vec4 vMajorColorBottom;

    // Цвет оси X при взгляде на нижнюю сторону сетки.
    glm::vec4 vXAxisColorBottom;

    // Цвет оси Y при взгляде на нижнюю сторону сетки.
    glm::vec4 vYAxisColorBottom;
};

// Данные текущего кадра, которые нужны сетке и вспомогательным renderer'ам.
struct SGridFrameData
{
    // View-матрица камеры.
    // Используется, например, для billboard-подписей маркера осей.
    glm::dmat4 mView;

    // View-projection матрица.
    // Используется для вычисления глубины и проекции точек.
    glm::dmat4 mViewProj;

    // Обратная view-projection матрица.
    // Используется vertex shader'ом сетки для восстановления near/far точек fullscreen quad.
    glm::dmat4 mInvViewProj;

    // Размер framebuffer в пикселях.
    // Используется для расчёта экранного масштаба adaptive grid.
    glm::dvec2 vViewportSize;

    // true, если текущий кадр рендерится в ортографической проекции.
    // Используется маркером начала координат.
    bool bIsOrthographicProjection;
};

// Renderer аналитической координатной сетки.
//
// Геометрически renderer рисует один fullscreen quad.
// Сама сетка вычисляется в shader program аналитически:
// каждый фрагмент определяет, попал ли он в плоскость сетки,
// в линию, в узел, в ось или в пустое место.
class CGridRenderer
{
public:
    // Создаёт renderer с дефолтной геометрией и стилем.
    CGridRenderer();

    // Освобождает OpenGL-ресурсы renderer'а.
    ~CGridRenderer();

    // Копирование и копирующее присваивание запрещено, потому что класс владеет OpenGL VAO.
    CGridRenderer(const CGridRenderer&) = delete;
    CGridRenderer& operator=(const CGridRenderer&) = delete;

    // Перемещающий конструктор передаёт владение OpenGL VAO.
    CGridRenderer(CGridRenderer&& other) noexcept;

    // Перемещающее присваивание передаёт владение OpenGL VAO.
    CGridRenderer& operator=(CGridRenderer&& other) noexcept;

    // Создаёт VAO для fullscreen triangle.
    // Вершинный буфер не нужен, потому что вершины генерируются через gl_VertexID.
    void Initialize();

    // Удаляет VAO, если он был создан.
    void Destroy();

    // Задаёт текущую геометрию сетки.
    void SetGeometry(const SGridGeometry& sGeometry);

    // Задаёт текущий стиль сетки.
    void SetStyle(const SGridStyle& sStyle);

    // Возвращает текущую геометрию сетки.
    const SGridGeometry& GetGeometry() const;

    // Возвращает текущий стиль сетки.
    const SGridStyle& GetStyle() const;

    // Обновляет adaptive step по текущей камере и viewport.
    void UpdateAdaptiveStep(const SGridFrameData& sFrameData);

    // Рисует сетку текущим shader program.
    // Перед вызовом метод устанавливает все uniform-параметры:
    // матрицы, геометрию сетки, шаги, цвета и режимы отображения.
    void Render(const CShaderProgram& shaderProgram, const SGridFrameData& sFrameData) const;

private:
    // VAO для fullscreen triangle.
    GLuint m_nFullscreenVao;

    // Текущая геометрия сетки.
    SGridGeometry m_sGeometry;

    // Текущий стиль и runtime-состояние сетки.
    SGridStyle m_sStyle;
};