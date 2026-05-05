#pragma once

#include "ShaderProgram.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

// Геометрические параметры сетки.
//
// Все основные координаты храним в double,
// потому что в CAD-сценах возможны большие мировые координаты.
struct SGridGeometry
{
    glm::dvec3 vOrigin;
    glm::dvec3 vAxisX;
    glm::dvec3 vAxisY;
    glm::dvec3 vNormal;
};

// Визуальные параметры сетки.
struct SGridStyle
{
    double dMinorStep;
    double dMajorStep;

    float fMinorThickness;
    float fMajorThickness;
    float fAxisThickness;

    double dMinViewNormalDot;

    // Управляемый режим:
    // false — фрагменты за near/far отбрасываются;
    // true  — depth прижимается к [0; 1].
    bool bClampDepth;

    bool bIsBounded;
    glm::dvec4 vBounds;

    bool bDrawDots;
    float fDotRadius;

    glm::vec4 vPlaneColorTop;
    glm::vec4 vMinorColorTop;
    glm::vec4 vMajorColorTop;
    glm::vec4 vXAxisColorTop;
    glm::vec4 vYAxisColorTop;

    glm::vec4 vPlaneColorBottom;
    glm::vec4 vMinorColorBottom;
    glm::vec4 vMajorColorBottom;
    glm::vec4 vXAxisColorBottom;
    glm::vec4 vYAxisColorBottom;
};

// Данные кадра, которые нужны сетке для рендера.
struct SGridFrameData
{
    glm::dmat4 mViewProj;
    glm::dmat4 mInvViewProj;
    glm::dvec2 vViewportSize;
};

// Renderer аналитической координатной сетки.
//
// Геометрически он рисует один fullscreen triangle.
// Реальная сетка вычисляется во fragment shader.
class CGridRenderer
{
public:
    CGridRenderer();
    ~CGridRenderer();

    CGridRenderer(const CGridRenderer&) = delete;
    CGridRenderer& operator=(const CGridRenderer&) = delete;

    CGridRenderer(CGridRenderer&& other) noexcept;
    CGridRenderer& operator=(CGridRenderer&& other) noexcept;

    void Initialize();
    void Destroy();

    void SetGeometry(const SGridGeometry& sGeometry);
    void SetStyle(const SGridStyle& sStyle);

    const SGridGeometry& GetGeometry() const;
    const SGridStyle& GetStyle() const;

    void Render(const CShaderProgram& shaderProgram, const SGridFrameData& sFrameData) const;

private:
    GLuint m_nFullscreenVao;

    SGridGeometry m_sGeometry;
    SGridStyle m_sStyle;
};