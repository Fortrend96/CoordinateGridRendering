#include "GridRenderer.h"

CGridRenderer::CGridRenderer()
    : m_nFullscreenVao(0)
{
    m_sGeometry.vOrigin = glm::dvec3(0.0, 0.0, 0.0);
    m_sGeometry.vAxisX = glm::dvec3(1.0, 0.0, 0.0);
    m_sGeometry.vAxisY = glm::dvec3(0.0, 1.0, 0.0);
    m_sGeometry.vNormal = glm::dvec3(0.0, 0.0, 1.0);

    m_sStyle.dMinorStep = 1.0;
    m_sStyle.dMajorStep = 10.0;

    m_sStyle.fMinorThickness = 0.9f;
    m_sStyle.fMajorThickness = 1.2f;
    m_sStyle.fAxisThickness = 1.8f;

    // sin(5°) ~= 0.087.
    // Если abs(dot(rayDir, normal)) меньше этого значения,
    // значит смотрим почти вдоль плоскости.
    m_sStyle.dMinViewNormalDot = 0.087;

    m_sStyle.bClampDepth = false;

    // По умолчанию заливка плоскости выключена.
    // Фон рисуется через glClearColor, а шейдер сетки выводит только линии.
    m_sStyle.bDrawPlane = false;

    m_sStyle.bIsBounded = false;
    m_sStyle.vBounds = glm::dvec4(-25.0, -25.0, 25.0, 25.0);

    m_sStyle.bDrawDots = false;
    m_sStyle.fDotRadius = 2.0f;

    // Верхняя сторона.
    m_sStyle.vPlaneColorTop = glm::vec4(0.145f, 0.176f, 0.223f, 1.00f);
    m_sStyle.vMinorColorTop = glm::vec4(0.215f, 0.250f, 0.305f, 0.72f);
    m_sStyle.vMajorColorTop = glm::vec4(0.305f, 0.355f, 0.430f, 0.92f);
    m_sStyle.vXAxisColorTop = glm::vec4(0.860f, 0.170f, 0.180f, 1.00f);
    m_sStyle.vYAxisColorTop = glm::vec4(0.180f, 0.730f, 0.300f, 1.00f);

    // Нижняя сторона.
    m_sStyle.vPlaneColorBottom = glm::vec4(0.125f, 0.152f, 0.192f, 1.00f);
    m_sStyle.vMinorColorBottom = glm::vec4(0.180f, 0.210f, 0.260f, 0.62f);
    m_sStyle.vMajorColorBottom = glm::vec4(0.255f, 0.300f, 0.370f, 0.82f);
    m_sStyle.vXAxisColorBottom = glm::vec4(0.620f, 0.120f, 0.130f, 0.95f);
    m_sStyle.vYAxisColorBottom = glm::vec4(0.130f, 0.560f, 0.230f, 0.95f);
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
    // Плохой угол обзора и режим глубины.
    shaderProgram.SetUniform1d("uMinViewNormalDot", m_sStyle.dMinViewNormalDot);
    shaderProgram.SetUniform1i("uClampDepth", m_sStyle.bClampDepth ? 1 : 0);
    shaderProgram.SetUniform1i("uDrawPlane", m_sStyle.bDrawPlane ? 1 : 0);
    
    // Infinite / bounded.
    shaderProgram.SetUniform1i("uIsBounded", m_sStyle.bIsBounded ? 1 : 0);
    shaderProgram.SetUniformVec4d("uGridBounds", m_sStyle.vBounds);

    // Lines / dots.
    shaderProgram.SetUniform1i("uDrawDots", m_sStyle.bDrawDots ? 1 : 0);
    shaderProgram.SetUniform1f("uDotRadius", m_sStyle.fDotRadius);

    // Шаги.
    shaderProgram.SetUniform1d("uMinorStep", m_sStyle.dMinorStep);
    shaderProgram.SetUniform1d("uMajorStep", m_sStyle.dMajorStep);

    // Толщины.
    shaderProgram.SetUniform1f("uMinorThickness", m_sStyle.fMinorThickness);
    shaderProgram.SetUniform1f("uMajorThickness", m_sStyle.fMajorThickness);
    shaderProgram.SetUniform1f("uAxisThickness", m_sStyle.fAxisThickness);

    // Верхняя сторона.
    shaderProgram.SetUniformVec4f("uPlaneColorTop", m_sStyle.vPlaneColorTop);
    shaderProgram.SetUniformVec4f("uMinorColorTop", m_sStyle.vMinorColorTop);
    shaderProgram.SetUniformVec4f("uMajorColorTop", m_sStyle.vMajorColorTop);
    shaderProgram.SetUniformVec4f("uXAxisColorTop", m_sStyle.vXAxisColorTop);
    shaderProgram.SetUniformVec4f("uYAxisColorTop", m_sStyle.vYAxisColorTop);

    // Нижняя сторона.
    shaderProgram.SetUniformVec4f("uPlaneColorBottom", m_sStyle.vPlaneColorBottom);
    shaderProgram.SetUniformVec4f("uMinorColorBottom", m_sStyle.vMinorColorBottom);
    shaderProgram.SetUniformVec4f("uMajorColorBottom", m_sStyle.vMajorColorBottom);
    shaderProgram.SetUniformVec4f("uXAxisColorBottom", m_sStyle.vXAxisColorBottom);
    shaderProgram.SetUniformVec4f("uYAxisColorBottom", m_sStyle.vYAxisColorBottom);

    glBindVertexArray(m_nFullscreenVao);
    // Один fullscreen triangle.
    // Реальная сетка вычисляется во fragment shader.
    glDrawArrays(GL_TRIANGLES, 0, 3);
}