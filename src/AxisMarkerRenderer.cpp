#include "AxisMarkerRenderer.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>

namespace
{
    struct SAxisMarkerVertex
    {
        glm::dvec3 vLocalPosition;
        glm::vec4 vColor;
    };

    void AppendLine(
        std::vector<SAxisMarkerVertex>& arrVertices,
        const glm::dvec3& vBegin,
        const glm::dvec3& vEnd,
        const glm::vec4& vColor
    )
    {
        arrVertices.push_back({ vBegin, vColor });
        arrVertices.push_back({ vEnd, vColor });
    }

    glm::dvec2 ProjectLocalPointToScreen(
        const glm::dmat4& mViewProj,
        const glm::dvec3& vGridOrigin,
        const glm::dvec3& vLocalPosition,
        const glm::dvec2& vViewportSize
    )
    {
        const glm::dvec3 vWorldPosition = vGridOrigin + vLocalPosition;
        const glm::dvec4 vClipPosition = mViewProj * glm::dvec4(vWorldPosition, 1.0);

        if (std::abs(vClipPosition.w) < 1e-12)
        {
            return glm::dvec2(0.0, 0.0);
        }

        const glm::dvec3 vNdc = glm::dvec3(vClipPosition) / vClipPosition.w;

        return glm::dvec2(
            (vNdc.x * 0.5 + 0.5) * vViewportSize.x,
            (vNdc.y * 0.5 + 0.5) * vViewportSize.y
        );
    }

    double GetPixelsPerWorldUnit(
        const glm::dmat4& mViewProj,
        const glm::dvec3& vGridOrigin,
        const glm::dvec3& vLocalDirection,
        const glm::dvec2& vViewportSize
    )
    {
        const glm::dvec3 vDirection = glm::normalize(vLocalDirection);

        const glm::dvec2 vScreenOrigin = ProjectLocalPointToScreen(
            mViewProj,
            vGridOrigin,
            glm::dvec3(0.0, 0.0, 0.0),
            vViewportSize
        );

        const glm::dvec2 vScreenUnit = ProjectLocalPointToScreen(
            mViewProj,
            vGridOrigin,
            vDirection,
            vViewportSize
        );

        const double dPixelsPerUnit = glm::length(vScreenUnit - vScreenOrigin);

        return std::max(dPixelsPerUnit, 1e-6);
    }

    double PixelsToWorldUnits(
        double dPixels,
        double dPixelsPerWorldUnit
    )
    {
        return dPixels / std::max(dPixelsPerWorldUnit, 1e-6);
    }

    void AppendGlyphX(
        std::vector<SAxisMarkerVertex>& arrVertices,
        const glm::dvec3& vCenter,
        const glm::dvec3& vBillboardRight,
        const glm::dvec3& vBillboardUp,
        double dHalfSizeWorld,
        const glm::vec4& vColor
    )
    {
        const glm::dvec3 vLeftBottom =
            vCenter - vBillboardRight * dHalfSizeWorld - vBillboardUp * dHalfSizeWorld;

        const glm::dvec3 vRightTop =
            vCenter + vBillboardRight * dHalfSizeWorld + vBillboardUp * dHalfSizeWorld;

        const glm::dvec3 vLeftTop =
            vCenter - vBillboardRight * dHalfSizeWorld + vBillboardUp * dHalfSizeWorld;

        const glm::dvec3 vRightBottom =
            vCenter + vBillboardRight * dHalfSizeWorld - vBillboardUp * dHalfSizeWorld;

        AppendLine(arrVertices, vLeftBottom, vRightTop, vColor);
        AppendLine(arrVertices, vLeftTop, vRightBottom, vColor);
    }

    void AppendGlyphY(
        std::vector<SAxisMarkerVertex>& arrVertices,
        const glm::dvec3& vCenter,
        const glm::dvec3& vBillboardRight,
        const glm::dvec3& vBillboardUp,
        double dHalfSizeWorld,
        const glm::vec4& vColor
    )
    {
        const glm::dvec3 vLeftTop =
            vCenter - vBillboardRight * dHalfSizeWorld + vBillboardUp * dHalfSizeWorld;

        const glm::dvec3 vRightTop =
            vCenter + vBillboardRight * dHalfSizeWorld + vBillboardUp * dHalfSizeWorld;

        const glm::dvec3 vMiddle = vCenter;

        const glm::dvec3 vBottom =
            vCenter - vBillboardUp * dHalfSizeWorld;

        AppendLine(arrVertices, vLeftTop, vMiddle, vColor);
        AppendLine(arrVertices, vRightTop, vMiddle, vColor);
        AppendLine(arrVertices, vMiddle, vBottom, vColor);
    }

    void AppendGlyphZ(
        std::vector<SAxisMarkerVertex>& arrVertices,
        const glm::dvec3& vCenter,
        const glm::dvec3& vBillboardRight,
        const glm::dvec3& vBillboardUp,
        double dHalfSizeWorld,
        const glm::vec4& vColor
    )
    {
        const glm::dvec3 vLeftTop =
            vCenter - vBillboardRight * dHalfSizeWorld + vBillboardUp * dHalfSizeWorld;

        const glm::dvec3 vRightTop =
            vCenter + vBillboardRight * dHalfSizeWorld + vBillboardUp * dHalfSizeWorld;

        const glm::dvec3 vLeftBottom =
            vCenter - vBillboardRight * dHalfSizeWorld - vBillboardUp * dHalfSizeWorld;

        const glm::dvec3 vRightBottom =
            vCenter + vBillboardRight * dHalfSizeWorld - vBillboardUp * dHalfSizeWorld;

        AppendLine(arrVertices, vLeftTop, vRightTop, vColor);
        AppendLine(arrVertices, vRightTop, vLeftBottom, vColor);
        AppendLine(arrVertices, vLeftBottom, vRightBottom, vColor);
    }

    void BuildAxisMarkerVertices(
        const SGridGeometry& sGridGeometry,
        const SGridFrameData& sFrameData,
        const SAxisMarkerStyle& sStyle,
        const glm::dvec3& vCameraRight,
        const glm::dvec3& vCameraUp,
        std::vector<SAxisMarkerVertex>& arrVertices
    )
    {
        arrVertices.clear();

        const glm::dvec3 vOriginLocal(0.0, 0.0, 0.0);

        const glm::dvec3 vAxisX = glm::normalize(sGridGeometry.vAxisX);
        const glm::dvec3 vAxisY = glm::normalize(sGridGeometry.vAxisY);
        const glm::dvec3 vAxisZ = glm::normalize(sGridGeometry.vNormal);

        // Считаем, сколько пикселей занимает 1 world unit около origin
        // в направлении каждой оси. Это позволяет задать длину маркера
        // в пикселях и получить world-space длину для текущего zoom.
        const double dPixelsPerUnitX = GetPixelsPerWorldUnit(
            sFrameData.mViewProj,
            sGridGeometry.vOrigin,
            vAxisX,
            sFrameData.vViewportSize
        );

        const double dPixelsPerUnitY = GetPixelsPerWorldUnit(
            sFrameData.mViewProj,
            sGridGeometry.vOrigin,
            vAxisY,
            sFrameData.vViewportSize
        );

        const double dPixelsPerUnitZ = GetPixelsPerWorldUnit(
            sFrameData.mViewProj,
            sGridGeometry.vOrigin,
            vAxisZ,
            sFrameData.vViewportSize
        );

        const double dPixelsPerUnitBillboardRight = GetPixelsPerWorldUnit(
            sFrameData.mViewProj,
            sGridGeometry.vOrigin,
            vCameraRight,
            sFrameData.vViewportSize
        );

        const double dPixelsPerUnitBillboardUp = GetPixelsPerWorldUnit(
            sFrameData.mViewProj,
            sGridGeometry.vOrigin,
            vCameraUp,
            sFrameData.vViewportSize
        );

        const double dPixelsPerUnitBillboard =
            std::max(
                (dPixelsPerUnitBillboardRight + dPixelsPerUnitBillboardUp) * 0.5,
                1e-6
            );

        const double dAxisLengthXWorld = PixelsToWorldUnits(
            sStyle.dAxisLengthPixels,
            dPixelsPerUnitX
        );

        const double dAxisLengthYWorld = PixelsToWorldUnits(
            sStyle.dAxisLengthPixels,
            dPixelsPerUnitY
        );

        const double dAxisLengthZWorld = PixelsToWorldUnits(
            sStyle.dAxisLengthPixels,
            dPixelsPerUnitZ
        );

        const double dLetterOffsetWorld = PixelsToWorldUnits(
            sStyle.dLetterOffsetPixels,
            dPixelsPerUnitBillboard
        );

        const double dLetterHalfSizeWorld = PixelsToWorldUnits(
            sStyle.dLetterSizePixels,
            dPixelsPerUnitBillboard
        );

        const glm::dvec3 vXEnd = vAxisX * dAxisLengthXWorld;
        const glm::dvec3 vYEnd = vAxisY * dAxisLengthYWorld;
        const glm::dvec3 vZEnd = vAxisZ * dAxisLengthZWorld;

        // Короткие оси из одной точки.
        // Их экранная длина остаётся примерно постоянной при zoom.
        AppendLine(arrVertices, vOriginLocal, vXEnd, sStyle.vAxisColor);
        AppendLine(arrVertices, vOriginLocal, vYEnd, sStyle.vAxisColor);
        AppendLine(arrVertices, vOriginLocal, vZEnd, sStyle.vAxisColor);

        // Центры букв располагаем возле концов соответствующих осей.
        const glm::dvec3 vXLabelCenter =
            vAxisX * (dAxisLengthXWorld + dLetterOffsetWorld);

        const glm::dvec3 vYLabelCenter =
            vAxisY * (dAxisLengthYWorld + dLetterOffsetWorld);

        const glm::dvec3 vZLabelCenter =
            vAxisZ * (dAxisLengthZWorld + dLetterOffsetWorld);

        // Буквы строятся в billboard-базисе камеры,
        // поэтому всегда смотрят на пользователя.
        AppendGlyphX(
            arrVertices,
            vXLabelCenter,
            vCameraRight,
            vCameraUp,
            dLetterHalfSizeWorld,
            sStyle.vTextColor
        );

        AppendGlyphY(
            arrVertices,
            vYLabelCenter,
            vCameraRight,
            vCameraUp,
            dLetterHalfSizeWorld,
            sStyle.vTextColor
        );

        AppendGlyphZ(
            arrVertices,
            vZLabelCenter,
            vCameraRight,
            vCameraUp,
            dLetterHalfSizeWorld,
            sStyle.vTextColor
        );
    }
}

CAxisMarkerRenderer::CAxisMarkerRenderer()
    : m_nVao(0)
    , m_nVbo(0)
{
    // Все размеры — в пикселях.
    // Поэтому маркер сохраняет визуальный размер при zoom.
    m_sStyle.dAxisLengthPixels = 48.0;
    m_sStyle.dLetterOffsetPixels = 10.0;
    m_sStyle.dLetterSizePixels = 6.0;

    m_sStyle.fLineWidth = 1.5f;

    m_sStyle.vAxisColor = glm::vec4(0.92f, 0.92f, 0.92f, 1.0f);
    m_sStyle.vTextColor = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
}

CAxisMarkerRenderer::~CAxisMarkerRenderer()
{
    Destroy();
}

CAxisMarkerRenderer::CAxisMarkerRenderer(CAxisMarkerRenderer&& other) noexcept
    : m_nVao(other.m_nVao)
    , m_nVbo(other.m_nVbo)
    , m_sStyle(other.m_sStyle)
{
    other.m_nVao = 0;
    other.m_nVbo = 0;
}

CAxisMarkerRenderer& CAxisMarkerRenderer::operator=(CAxisMarkerRenderer&& other) noexcept
{
    if (this != &other)
    {
        Destroy();

        m_nVao = other.m_nVao;
        m_nVbo = other.m_nVbo;
        m_sStyle = other.m_sStyle;

        other.m_nVao = 0;
        other.m_nVbo = 0;
    }

    return *this;
}

void CAxisMarkerRenderer::Initialize()
{
    if (m_nVao != 0)
    {
        return;
    }

    glGenVertexArrays(1, &m_nVao);
    glGenBuffers(1, &m_nVbo);

    glBindVertexArray(m_nVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_nVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribLPointer(
        0,
        3,
        GL_DOUBLE,
        sizeof(SAxisMarkerVertex),
        reinterpret_cast<const void*>(offsetof(SAxisMarkerVertex, vLocalPosition))
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(SAxisMarkerVertex),
        reinterpret_cast<const void*>(offsetof(SAxisMarkerVertex, vColor))
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void CAxisMarkerRenderer::Destroy()
{
    if (m_nVbo != 0)
    {
        glDeleteBuffers(1, &m_nVbo);
        m_nVbo = 0;
    }

    if (m_nVao != 0)
    {
        glDeleteVertexArrays(1, &m_nVao);
        m_nVao = 0;
    }
}

void CAxisMarkerRenderer::SetStyle(const SAxisMarkerStyle& sStyle)
{
    m_sStyle = sStyle;
}

const SAxisMarkerStyle& CAxisMarkerRenderer::GetStyle() const
{
    return m_sStyle;
}

void CAxisMarkerRenderer::Render(
    const CShaderProgram& shaderProgram,
    const SGridFrameData& sFrameData,
    const SGridGeometry& sGridGeometry
) const
{
    const glm::dmat4 mInvView = glm::inverse(sFrameData.mView);

    const glm::dvec3 vCameraRight = glm::normalize(glm::dvec3(mInvView[0]));
    const glm::dvec3 vCameraUp = glm::normalize(glm::dvec3(mInvView[1]));

    std::vector<SAxisMarkerVertex> arrVertices;

    BuildAxisMarkerVertices(
        sGridGeometry,
        sFrameData,
        m_sStyle,
        vCameraRight,
        vCameraUp,
        arrVertices
    );

    if (arrVertices.empty())
    {
        return;
    }

    shaderProgram.Use();

    shaderProgram.SetUniformMat4d("uViewProj", sFrameData.mViewProj);
    shaderProgram.SetUniformVec3d("uGridOrigin", sGridGeometry.vOrigin);

    glBindVertexArray(m_nVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_nVbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(arrVertices.size() * sizeof(SAxisMarkerVertex)),
        arrVertices.data(),
        GL_DYNAMIC_DRAW
    );

    const GLboolean bDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);

    if (bDepthTestEnabled == GL_TRUE)
    {
        glDisable(GL_DEPTH_TEST);
    }

    glLineWidth(m_sStyle.fLineWidth);

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(arrVertices.size()));

    glLineWidth(1.0f);

    if (bDepthTestEnabled == GL_TRUE)
    {
        glEnable(GL_DEPTH_TEST);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}