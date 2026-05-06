#pragma once

#include "GridRenderer.h"
#include "ShaderProgram.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

// Визуальные параметры маркера начала координат.
//
// Важно:
// размеры задаются в пикселях, а не в мировых единицах.
// Это нужно, чтобы маркер не менял визуальный размер при zoom.
struct SAxisMarkerStyle
{
    double dAxisLengthPixels;
    double dLetterOffsetPixels;
    double dLetterSizePixels;

    float fLineWidth;

    glm::vec4 vAxisColor;
    glm::vec4 vTextColor;
};

class CAxisMarkerRenderer
{
public:
    CAxisMarkerRenderer();
    ~CAxisMarkerRenderer();

    CAxisMarkerRenderer(const CAxisMarkerRenderer&) = delete;
    CAxisMarkerRenderer& operator=(const CAxisMarkerRenderer&) = delete;

    CAxisMarkerRenderer(CAxisMarkerRenderer&& other) noexcept;
    CAxisMarkerRenderer& operator=(CAxisMarkerRenderer&& other) noexcept;

    void Initialize();
    void Destroy();

    void SetStyle(const SAxisMarkerStyle& sStyle);
    const SAxisMarkerStyle& GetStyle() const;

    void Render(
        const CShaderProgram& shaderProgram,
        const SGridFrameData& sFrameData,
        const SGridGeometry& sGridGeometry
    ) const;

private:
    GLuint m_nVao;
    GLuint m_nVbo;

    SAxisMarkerStyle m_sStyle;
};