#include "GridRenderer.h"

#include <algorithm>
#include <cmath>

namespace
{
    glm::dvec3 SafeNormalize(
        const glm::dvec3& vVector,
        const glm::dvec3& vFallback
    )
    {
        const double dLength = glm::length(vVector);

        if (dLength <= 1e-12)
        {
            return vFallback;
        }

        return vVector / dLength;
    }

    bool ProjectWorldPointToScreen(
        const glm::dmat4& mViewProj,
        const glm::dvec3& vWorldPosition,
        const glm::dvec2& vViewportSize,
        glm::dvec2& vScreenPosition
    )
    {
        const glm::dvec4 vClip = mViewProj * glm::dvec4(vWorldPosition, 1.0);

        if (std::abs(vClip.w) < 1e-12)
        {
            return false;
        }

        const glm::dvec3 vNdc = glm::dvec3(vClip) / vClip.w;

        vScreenPosition = glm::dvec2(
            (vNdc.x * 0.5 + 0.5) * vViewportSize.x,
            (vNdc.y * 0.5 + 0.5) * vViewportSize.y
        );

        return true;
    }

    double ComputePixelsPerWorldUnit(
        const glm::dmat4& mViewProj,
        const glm::dvec3& vOrigin,
        const glm::dvec3& vDirection,
        const glm::dvec2& vViewportSize
    )
    {
        glm::dvec2 vScreenOrigin(0.0);
        glm::dvec2 vScreenUnit(0.0);

        const glm::dvec3 vNormalizedDirection =
            SafeNormalize(vDirection, glm::dvec3(1.0, 0.0, 0.0));

        const bool bOriginProjected = ProjectWorldPointToScreen(
            mViewProj,
            vOrigin,
            vViewportSize,
            vScreenOrigin
        );

        const bool bUnitProjected = ProjectWorldPointToScreen(
            mViewProj,
            vOrigin + vNormalizedDirection,
            vViewportSize,
            vScreenUnit
        );

        if (!bOriginProjected || !bUnitProjected)
        {
            return 0.0;
        }

        return glm::length(vScreenUnit - vScreenOrigin);
    }

    double RoundUpToNiceStep(double dDesiredStep)
    {
        if (!std::isfinite(dDesiredStep) || dDesiredStep <= 0.0)
        {
            return 1.0;
        }

        const double dExponent = std::floor(std::log10(dDesiredStep));
        const double dBase = std::pow(10.0, dExponent);
        const double dMantissa = dDesiredStep / dBase;

        double dNiceMantissa = 10.0;

        if (dMantissa <= 1.0)
        {
            dNiceMantissa = 1.0;
        }
        else if (dMantissa <= 2.0)
        {
            dNiceMantissa = 2.0;
        }
        else if (dMantissa <= 5.0)
        {
            dNiceMantissa = 5.0;
        }

        return dNiceMantissa * dBase;
    }
}

CGridRenderer::CGridRenderer()
    : m_nFullscreenVao(0)
{
    m_sGeometry.vOrigin = glm::dvec3(0.0, 0.0, 0.0);
    m_sGeometry.vAxisX = glm::dvec3(1.0, 0.0, 0.0);
    m_sGeometry.vAxisY = glm::dvec3(0.0, 1.0, 0.0);
    m_sGeometry.vNormal = glm::dvec3(0.0, 0.0, 1.0);

    m_sStyle.dMinorStep = 1.0;
    m_sStyle.nMajorLineFrequency = 10;

    m_sStyle.fMinorThickness = 0.75f;
    m_sStyle.fMajorThickness = 1.05f;
    m_sStyle.fAxisThickness = 1.6f;

    m_sStyle.dMinViewNormalDot = 0.005;
    m_sStyle.dSafeDepthEpsilon = 0.000001;

    m_sStyle.bDebugDepthZones = false;

    // По умолчанию плоскость сетки слегка подкрашена.
    m_sStyle.bDrawPlane = true;

    m_sStyle.bIsBounded = false;
    m_sStyle.vBounds = glm::dvec4(-25.0, -25.0, 25.0, 25.0);

    m_sStyle.bDrawDots = false;
    m_sStyle.fDotRadius = 2.0f;

    m_sStyle.bUseAdaptiveStep = true;
    m_sStyle.dTargetMinorStepPixels = 28.0;

    m_sStyle.bShowMinorGrid = true;
    m_sStyle.bShowMajorGrid = true;
    m_sStyle.bShowAxes = true;

    // Цвета верхней стороны сетки.
    // Подобраны под тёмный CAD-like фон.
    m_sStyle.vPlaneColorTop = glm::vec4(0.115f, 0.135f, 0.160f, 0.22f);
    m_sStyle.vMinorColorTop = glm::vec4(0.250f, 0.285f, 0.325f, 0.22f);
    m_sStyle.vMajorColorTop = glm::vec4(0.390f, 0.430f, 0.480f, 0.62f);
    m_sStyle.vXAxisColorTop = glm::vec4(0.860f, 0.170f, 0.180f, 1.00f);
    m_sStyle.vYAxisColorTop = glm::vec4(0.180f, 0.730f, 0.300f, 1.00f);

    // Цвета нижней стороны сетки.
    m_sStyle.vPlaneColorBottom = glm::vec4(0.090f, 0.105f, 0.125f, 0.18f);
    m_sStyle.vMinorColorBottom = glm::vec4(0.200f, 0.230f, 0.270f, 0.18f);
    m_sStyle.vMajorColorBottom = glm::vec4(0.310f, 0.350f, 0.400f, 0.52f);
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

void CGridRenderer::UpdateAdaptiveStep(const SGridFrameData& sFrameData)
{
    if (!m_sStyle.bUseAdaptiveStep)
    {
        return;
    }

    const double dPixelsPerUnitX = ComputePixelsPerWorldUnit(
        sFrameData.mViewProj,
        m_sGeometry.vOrigin,
        m_sGeometry.vAxisX,
        sFrameData.vViewportSize
    );

    const double dPixelsPerUnitY = ComputePixelsPerWorldUnit(
        sFrameData.mViewProj,
        m_sGeometry.vOrigin,
        m_sGeometry.vAxisY,
        sFrameData.vViewportSize
    );

    double dPixelsPerWorldUnit = 0.0;

    if (dPixelsPerUnitX > 0.0 && dPixelsPerUnitY > 0.0)
    {
        dPixelsPerWorldUnit = std::min(dPixelsPerUnitX, dPixelsPerUnitY);
    }
    else
    {
        dPixelsPerWorldUnit = std::max(dPixelsPerUnitX, dPixelsPerUnitY);
    }

    if (!std::isfinite(dPixelsPerWorldUnit) || dPixelsPerWorldUnit <= 1e-9)
    {
        return;
    }

    // Подбираем только minor step.
    // Major step всегда вычисляется как minorStep * majorLineFrequency.
    const double dDesiredMinorStep =
        m_sStyle.dTargetMinorStepPixels / dPixelsPerWorldUnit;

    m_sStyle.dMinorStep = RoundUpToNiceStep(dDesiredMinorStep);

    const int nMajorLineFrequency =
        std::max(m_sStyle.nMajorLineFrequency, 1);

    const double dMajorStep =
        m_sStyle.dMinorStep * static_cast<double>(nMajorLineFrequency);

    const double dMinorStepPixels =
        m_sStyle.dMinorStep * dPixelsPerWorldUnit;

    const double dMajorStepPixels =
        dMajorStep * dPixelsPerWorldUnit;

    // Слишком плотную minor-сетку скрываем, чтобы уменьшить муар.
    m_sStyle.bShowMinorGrid = dMinorStepPixels >= 14.0;
    m_sStyle.bShowMajorGrid = dMajorStepPixels >= 8.0;
    m_sStyle.bShowAxes = true;
}

void CGridRenderer::Render(
    const CShaderProgram& shaderProgram,
    const SGridFrameData& sFrameData
) const
{
    shaderProgram.Use();

    // В shader передаём только projection/invProjection.
    // Луч пикселя восстанавливается в eye-space.
    shaderProgram.SetUniformMat4d("uProjection", sFrameData.mProjection);
    shaderProgram.SetUniformMat4d("uInvProjection", sFrameData.mInvProjection);
    shaderProgram.SetUniformVec2d("uViewportSize", sFrameData.vViewportSize);

    // Параметры сетки переводим из world-space в eye-space на CPU.
    const glm::dvec3 vOriginEye = glm::dvec3(
        sFrameData.mView * glm::dvec4(m_sGeometry.vOrigin, 1.0)
    );

    const glm::dvec3 vAxisXEye = SafeNormalize(
        glm::dvec3(sFrameData.mView * glm::dvec4(m_sGeometry.vAxisX, 0.0)),
        glm::dvec3(1.0, 0.0, 0.0)
    );

    const glm::dvec3 vAxisYEye = SafeNormalize(
        glm::dvec3(sFrameData.mView * glm::dvec4(m_sGeometry.vAxisY, 0.0)),
        glm::dvec3(0.0, 1.0, 0.0)
    );

    const glm::dvec3 vNormalEye = SafeNormalize(
        glm::cross(vAxisXEye, vAxisYEye),
        glm::dvec3(0.0, 0.0, 1.0)
    );

    shaderProgram.SetUniformVec3d("uGridOriginEye", vOriginEye);
    shaderProgram.SetUniformVec3d("uGridAxisXEye", vAxisXEye);
    shaderProgram.SetUniformVec3d("uGridAxisYEye", vAxisYEye);
    shaderProgram.SetUniformVec3d("uGridNormalEye", vNormalEye);

    shaderProgram.SetUniform1d("uMinViewNormalDot", m_sStyle.dMinViewNormalDot);
    shaderProgram.SetUniform1d("uSafeDepthEpsilon", m_sStyle.dSafeDepthEpsilon);
    shaderProgram.SetUniform1i("uDebugDepthZones", m_sStyle.bDebugDepthZones ? 1 : 0);

    shaderProgram.SetUniform1i("uDrawPlane", m_sStyle.bDrawPlane ? 1 : 0);

    shaderProgram.SetUniform1i("uIsBounded", m_sStyle.bIsBounded ? 1 : 0);
    shaderProgram.SetUniformVec4d("uGridBounds", m_sStyle.vBounds);

    shaderProgram.SetUniform1i("uDrawDots", m_sStyle.bDrawDots ? 1 : 0);

    shaderProgram.SetUniform1i("uShowMinorGrid", m_sStyle.bShowMinorGrid ? 1 : 0);
    shaderProgram.SetUniform1i("uShowMajorGrid", m_sStyle.bShowMajorGrid ? 1 : 0);
    shaderProgram.SetUniform1i("uShowAxes", m_sStyle.bShowAxes ? 1 : 0);

    const int nMajorLineFrequency =
        std::max(m_sStyle.nMajorLineFrequency, 1);

    const double dMajorStep =
        m_sStyle.dMinorStep * static_cast<double>(nMajorLineFrequency);

    shaderProgram.SetUniform1d("uMinorStep", m_sStyle.dMinorStep);
    shaderProgram.SetUniform1d("uMajorStep", dMajorStep);

    shaderProgram.SetUniform1f("uMinorThickness", m_sStyle.fMinorThickness);
    shaderProgram.SetUniform1f("uMajorThickness", m_sStyle.fMajorThickness);
    shaderProgram.SetUniform1f("uAxisThickness", m_sStyle.fAxisThickness);
    shaderProgram.SetUniform1f("uDotRadius", m_sStyle.fDotRadius);

    shaderProgram.SetUniformVec4f("uPlaneColorTop", m_sStyle.vPlaneColorTop);
    shaderProgram.SetUniformVec4f("uMinorColorTop", m_sStyle.vMinorColorTop);
    shaderProgram.SetUniformVec4f("uMajorColorTop", m_sStyle.vMajorColorTop);
    shaderProgram.SetUniformVec4f("uXAxisColorTop", m_sStyle.vXAxisColorTop);
    shaderProgram.SetUniformVec4f("uYAxisColorTop", m_sStyle.vYAxisColorTop);

    shaderProgram.SetUniformVec4f("uPlaneColorBottom", m_sStyle.vPlaneColorBottom);
    shaderProgram.SetUniformVec4f("uMinorColorBottom", m_sStyle.vMinorColorBottom);
    shaderProgram.SetUniformVec4f("uMajorColorBottom", m_sStyle.vMajorColorBottom);
    shaderProgram.SetUniformVec4f("uXAxisColorBottom", m_sStyle.vXAxisColorBottom);
    shaderProgram.SetUniformVec4f("uYAxisColorBottom", m_sStyle.vYAxisColorBottom);

    glBindVertexArray(m_nFullscreenVao);

    // Вершины fullscreen quad генерируются во vertex shader через gl_VertexID.
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}