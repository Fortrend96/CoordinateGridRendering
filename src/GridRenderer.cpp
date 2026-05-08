#include "GridRenderer.h"

#include <algorithm>
#include <cmath>

namespace
{
    // Проецирует world-space точку в screen-space.
    // Возвращает:
    // - true, если точку удалось корректно спроецировать;
    // - false, если clip.w слишком близок к нулю.
    // Используется не для рендера, а для расчёта экранного масштаба сетки.
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

    // Считает, сколько пикселей занимает 1 world unit около origin сетки
    // в заданном направлении.
    //
    // Например:
    // - берём origin;
    // - берём origin + axisX;
    // - проецируем обе точки на экран;
    // - расстояние между ними в пикселях и есть pixelsPerWorldUnit.
    //
    // Это значение используется adaptive grid'ом для выбора шага сетки.
    double ComputePixelsPerWorldUnit(
        const glm::dmat4& mViewProj,
        const glm::dvec3& vOrigin,
        const glm::dvec3& vDirection,
        const glm::dvec2& vViewportSize
    )
    {
        glm::dvec2 vScreenOrigin(0.0);
        glm::dvec2 vScreenUnit(0.0);

        const glm::dvec3 vNormalizedDirection = glm::normalize(vDirection);

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

    // Округляет желаемый шаг вверх.
    // Используется шкала:
    // ..., 0.1, 0.2, 0.5,
    // 1, 2, 5,
    // 10, 20, 50,
    // 100, ...
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
    m_sStyle.dMajorStep = 10.0;

    m_sStyle.fMinorThickness = 0.75f;
    m_sStyle.fMajorThickness = 1.05f;
    m_sStyle.fAxisThickness = 1.6f;

    // Минимально допустимый угол взгляда к плоскости сетки.
    // сетка скрывается только при почти касательном взгляде.
    m_sStyle.dMinViewNormalDot = 0.005;

    // Безопасный отступ от точных 0 и 1 в depth buffer.
    m_sStyle.dSafeDepthEpsilon = 0.000001;

    // Отладочная раскраска зон глубины по умолчанию выключена.
    m_sStyle.bDebugDepthZones = false;


    m_sStyle.bIsBounded = false;
    m_sStyle.vBounds = glm::dvec4(-25.0, -25.0, 25.0, 25.0);

    m_sStyle.bDrawDots = false;
    m_sStyle.fDotRadius = 2.0f;

    // Adaptive grid включён по умолчанию.
    m_sStyle.bUseAdaptiveStep = true;

    // Примерная желаемая плотность малой сетки.
    // 18 px даёт читаемую сетку без чрезмерной плотности.
    m_sStyle.dTargetMinorStepPixels = 28.0;

    // Каждая 10-я малая линия становится большой.
    m_sStyle.nMajorLineFrequency = 10;

    m_sStyle.bShowMinorGrid = true;
    m_sStyle.bShowMajorGrid = true;
    m_sStyle.bShowAxes = true;

    // Верхняя сторона.
    //
    // Это только дефолтные тестовые значения демо.
    // В целевом варианте все цвета приходят снаружи и уходят в shader uniform'ами.
    m_sStyle.vMinorColorTop = glm::vec4(0.215f, 0.250f, 0.305f, 0.72f);
    m_sStyle.vMajorColorTop = glm::vec4(0.305f, 0.355f, 0.430f, 0.92f);
    m_sStyle.vXAxisColorTop = glm::vec4(0.860f, 0.170f, 0.180f, 1.00f);
    m_sStyle.vYAxisColorTop = glm::vec4(0.180f, 0.730f, 0.300f, 1.00f);

    // Нижняя сторона.
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
    // Если adaptive step выключен, оставляем вручную заданные dMinorStep/dMajorStep.
    if (!m_sStyle.bUseAdaptiveStep)
    {
        return;
    }

    // Оцениваем экранный масштаб вдоль локальной оси X сетки.
    const double dPixelsPerUnitX = ComputePixelsPerWorldUnit(
        sFrameData.mViewProj,
        m_sGeometry.vOrigin,
        m_sGeometry.vAxisX,
        sFrameData.vViewportSize
    );

    // Оцениваем экранный масштаб вдоль локальной оси Y сетки.
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

    // Если оценить масштаб не удалось, не трогаем текущий шаг.
    if (!std::isfinite(dPixelsPerWorldUnit) || dPixelsPerWorldUnit <= 1e-9)
    {
        return;
    }

    // Переводим желаемое расстояние между линиями из пикселей в world units.
    const double dDesiredMinorStep =
        m_sStyle.dTargetMinorStepPixels / dPixelsPerWorldUnit;

    // Округляем шаг до красивого CAD-like значения.
    const double dMinorStep = RoundUpToNiceStep(dDesiredMinorStep);

    // Большой шаг строим как N малых шагов.
    const int nMajorLineFrequency = std::max(m_sStyle.nMajorLineFrequency, 1);
    const double dMajorStep = dMinorStep * static_cast<double>(nMajorLineFrequency);

    m_sStyle.dMinorStep = dMinorStep;
    m_sStyle.dMajorStep = dMajorStep;

    // Переводим выбранные шаги обратно в пиксели,
    // чтобы понять, стоит ли вообще показывать соответствующий слой.
    const double dMinorStepPixels = dMinorStep * dPixelsPerWorldUnit;
    const double dMajorStepPixels = dMajorStep * dPixelsPerWorldUnit;

    // Не рисуем minor-сетку, если она становится слишком плотной на экране.
    m_sStyle.bShowMinorGrid = dMinorStepPixels >= 14.0;

    // Major-сетка может быть чуть плотнее
    m_sStyle.bShowMajorGrid = dMajorStepPixels >= 8.0;

    // Оси X/Y оставляем видимыми всегда.
    m_sStyle.bShowAxes = true;
}

void CGridRenderer::Render(const CShaderProgram& shaderProgram, const SGridFrameData& sFrameData) const
{
    shaderProgram.Use();

    shaderProgram.SetUniformMat4d("uViewProj", sFrameData.mViewProj);
    shaderProgram.SetUniformMat4d("uInvViewProj", sFrameData.mInvViewProj);
    shaderProgram.SetUniformVec2d("uViewportSize", sFrameData.vViewportSize);

    shaderProgram.SetUniformVec3d("uGridOrigin", m_sGeometry.vOrigin);
    shaderProgram.SetUniformVec3d("uGridAxisX", m_sGeometry.vAxisX);
    shaderProgram.SetUniformVec3d("uGridAxisY", m_sGeometry.vAxisY);
    shaderProgram.SetUniformVec3d("uGridNormal", m_sGeometry.vNormal);

    shaderProgram.SetUniform1d("uMinViewNormalDot", m_sStyle.dMinViewNormalDot);

    shaderProgram.SetUniform1d("uSafeDepthEpsilon", m_sStyle.dSafeDepthEpsilon);
    shaderProgram.SetUniform1i("uDebugDepthZones", m_sStyle.bDebugDepthZones ? 1 : 0);

    shaderProgram.SetUniform1i("uIsBounded", m_sStyle.bIsBounded ? 1 : 0);
    shaderProgram.SetUniformVec4d("uGridBounds", m_sStyle.vBounds);

    shaderProgram.SetUniform1i("uDrawDots", m_sStyle.bDrawDots ? 1 : 0);

    shaderProgram.SetUniform1i("uShowMinorGrid", m_sStyle.bShowMinorGrid ? 1 : 0);
    shaderProgram.SetUniform1i("uShowMajorGrid", m_sStyle.bShowMajorGrid ? 1 : 0);
    shaderProgram.SetUniform1i("uShowAxes", m_sStyle.bShowAxes ? 1 : 0);

    shaderProgram.SetUniform1f("uDotRadius", m_sStyle.fDotRadius);

    shaderProgram.SetUniform1d("uMinorStep", m_sStyle.dMinorStep);
    shaderProgram.SetUniform1d("uMajorStep", m_sStyle.dMajorStep);

    shaderProgram.SetUniform1f("uMinorThickness", m_sStyle.fMinorThickness);
    shaderProgram.SetUniform1f("uMajorThickness", m_sStyle.fMajorThickness);
    shaderProgram.SetUniform1f("uAxisThickness", m_sStyle.fAxisThickness);

    shaderProgram.SetUniformVec4f("uMinorColorTop", m_sStyle.vMinorColorTop);
    shaderProgram.SetUniformVec4f("uMajorColorTop", m_sStyle.vMajorColorTop);
    shaderProgram.SetUniformVec4f("uXAxisColorTop", m_sStyle.vXAxisColorTop);
    shaderProgram.SetUniformVec4f("uYAxisColorTop", m_sStyle.vYAxisColorTop);

    shaderProgram.SetUniformVec4f("uMinorColorBottom", m_sStyle.vMinorColorBottom);
    shaderProgram.SetUniformVec4f("uMajorColorBottom", m_sStyle.vMajorColorBottom);
    shaderProgram.SetUniformVec4f("uXAxisColorBottom", m_sStyle.vXAxisColorBottom);
    shaderProgram.SetUniformVec4f("uYAxisColorBottom", m_sStyle.vYAxisColorBottom);

    glBindVertexArray(m_nFullscreenVao);

    // Fullscreen quad рисуется двумя треугольниками.
    glDrawArrays(GL_TRIANGLES, 0, 6);
}