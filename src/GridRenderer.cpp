#include "GridRenderer.h"

CGridRenderer::CGridRenderer()
    : m_nFullscreenVao(0)
{
    // Значения по умолчанию: обычная сетка в плоскости Z = 0.
    m_sGeometry.vOrigin = glm::dvec3(0.0, 0.0, 0.0);
    m_sGeometry.vAxisX = glm::dvec3(1.0, 0.0, 0.0);
    m_sGeometry.vAxisY = glm::dvec3(0.0, 1.0, 0.0);
    m_sGeometry.vNormal = glm::dvec3(0.0, 0.0, 1.0);

    m_sStyle.dMinorStep = 1.0;
    m_sStyle.dMajorStep = 10.0;

    m_sStyle.fMinorThickness = 1.0f;
    m_sStyle.fMajorThickness = 1.5f;
    m_sStyle.fAxisThickness = 2.5f;

    // sin(5°) ~= 0.087.
    // Если abs(dot(rayDir, normal)) меньше этого значения,
    // значит смотрим почти вдоль плоскости.
    m_sStyle.dMinViewNormalDot = 0.087;

    m_sStyle.bIsBounded = false;
    m_sStyle.vBounds = glm::dvec4(-25.0, -25.0, 25.0, 25.0);

    m_sStyle.bDrawDots = false;
    m_sStyle.fDotRadius = 2.0f;

    m_sStyle.vPlaneColor = glm::vec4(0.03f, 0.03f, 0.035f, 0.35f);
    m_sStyle.vMinorColor = glm::vec4(0.32f, 0.32f, 0.34f, 0.55f);
    m_sStyle.vMajorColor = glm::vec4(0.58f, 0.58f, 0.62f, 0.75f);
    m_sStyle.vXAxisColor = glm::vec4(0.95f, 0.12f, 0.12f, 0.95f);
    m_sStyle.vYAxisColor = glm::vec4(0.15f, 0.85f, 0.20f, 0.95f);
}

CGridRenderer::~CGridRenderer()
{
    Destroy();
}

CGridRenderer::CGridRenderer(CGridRenderer&& other) noexcept
    : m_nFullscreenVao(other.m_nFullscreenVao)
    , m_sGeometry(other.m_sGeometry)
    , m_sStyle(other.m_sStyle)
{
    other.m_nFullscreenVao = 0;
}

CGridRenderer& CGridRenderer::operator=(CGridRenderer&& other) noexcept
{
    if (this != &other)
    {
        Destroy();

        m_nFullscreenVao = other.m_nFullscreenVao;
        m_sGeometry = other.m_sGeometry;
        m_sStyle = other.m_sStyle;

        other.m_nFullscreenVao = 0;
    }

    return *this;
}

void CGridRenderer::Initialize()
{
    if (m_nFullscreenVao != 0)
    {
        return;
    }

    // В OpenGL Core Profile VAO нужен даже тогда,
    // когда vertex buffer не используется.
    //
    // Вершины fullscreen triangle создаются в vertex shader
    // через gl_VertexID.
    glGenVertexArrays(1, &m_nFullscreenVao);
}

void CGridRenderer::Destroy()
{
    if (m_nFullscreenVao != 0)
    {
        glDeleteVertexArrays(1, &m_nFullscreenVao);
        m_nFullscreenVao = 0;
    }
}

void CGridRenderer::SetGeometry(const SGridGeometry& sGeometry)
{
    m_sGeometry = sGeometry;
}

void CGridRenderer::SetStyle(const SGridStyle& sStyle)
{
    m_sStyle = sStyle;
}

const SGridGeometry& CGridRenderer::GetGeometry() const
{
    return m_sGeometry;
}

const SGridStyle& CGridRenderer::GetStyle() const
{
    return m_sStyle;
}

void CGridRenderer::Render(const CShaderProgram& shaderProgram, const SGridFrameData& sFrameData) const
{
    shaderProgram.Use();

    // Матрицы и размер viewport.
    shaderProgram.SetUniformMat4d("uViewProj", sFrameData.mViewProj);
    shaderProgram.SetUniformMat4d("uInvViewProj", sFrameData.mInvViewProj);
    shaderProgram.SetUniformVec2d("uViewportSize", sFrameData.vViewportSize);

    // Геометрия сетки.
    shaderProgram.SetUniformVec3d("uGridOrigin", m_sGeometry.vOrigin);
    shaderProgram.SetUniformVec3d("uGridAxisX", m_sGeometry.vAxisX);
    shaderProgram.SetUniformVec3d("uGridAxisY", m_sGeometry.vAxisY);
    shaderProgram.SetUniformVec3d("uGridNormal", m_sGeometry.vNormal);

    shaderProgram.SetUniform1i("uIsBounded", m_sStyle.bIsBounded ? 1 : 0);
    shaderProgram.SetUniformVec4d("uGridBounds", m_sStyle.vBounds);

    shaderProgram.SetUniform1i("uDrawDots", m_sStyle.bDrawDots ? 1 : 0);
    shaderProgram.SetUniform1f("uDotRadius", m_sStyle.fDotRadius);

    // Визуальные параметры.
    shaderProgram.SetUniform1d("uMinViewNormalDot", m_sStyle.dMinViewNormalDot);

    shaderProgram.SetUniform1d("uMinorStep", m_sStyle.dMinorStep);
    shaderProgram.SetUniform1d("uMajorStep", m_sStyle.dMajorStep);

    shaderProgram.SetUniform1f("uMinorThickness", m_sStyle.fMinorThickness);
    shaderProgram.SetUniform1f("uMajorThickness", m_sStyle.fMajorThickness);
    shaderProgram.SetUniform1f("uAxisThickness", m_sStyle.fAxisThickness);

    shaderProgram.SetUniformVec4f("uPlaneColor", m_sStyle.vPlaneColor);
    shaderProgram.SetUniformVec4f("uMinorColor", m_sStyle.vMinorColor);
    shaderProgram.SetUniformVec4f("uMajorColor", m_sStyle.vMajorColor);
    shaderProgram.SetUniformVec4f("uXAxisColor", m_sStyle.vXAxisColor);
    shaderProgram.SetUniformVec4f("uYAxisColor", m_sStyle.vYAxisColor);

    glBindVertexArray(m_nFullscreenVao);

    // Один fullscreen triangle.
    // Реальная сетка вычисляется во fragment shader.
    glDrawArrays(GL_TRIANGLES, 0, 3);
}