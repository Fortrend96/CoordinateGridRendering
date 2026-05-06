#include "AxisMarkerRenderer.h"

#include <cstddef>
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

    void AppendGlyphX(
        std::vector<SAxisMarkerVertex>& arrVertices,
        const glm::dvec3& vCenter,
        const glm::dvec3& vBillboardRight,
        const glm::dvec3& vBillboardUp,
        double dSize,
        const glm::vec4& vColor
    )
    {
        const glm::dvec3 vLeftBottom = vCenter - vBillboardRight * dSize - vBillboardUp * dSize;
        const glm::dvec3 vRightTop = vCenter + vBillboardRight * dSize + vBillboardUp * dSize;

        const glm::dvec3 vLeftTop = vCenter - vBillboardRight * dSize + vBillboardUp * dSize;
        const glm::dvec3 vRightBottom = vCenter + vBillboardRight * dSize - vBillboardUp * dSize;

        AppendLine(arrVertices, vLeftBottom, vRightTop, vColor);
        AppendLine(arrVertices, vLeftTop, vRightBottom, vColor);
    }

    void AppendGlyphY(
        std::vector<SAxisMarkerVertex>& arrVertices,
        const glm::dvec3& vCenter,
        const glm::dvec3& vBillboardRight,
        const glm::dvec3& vBillboardUp,
        double dSize,
        const glm::vec4& vColor
    )
    {
        const glm::dvec3 vLeftTop = vCenter - vBillboardRight * dSize + vBillboardUp * dSize;
        const glm::dvec3 vRightTop = vCenter + vBillboardRight * dSize + vBillboardUp * dSize;
        const glm::dvec3 vMiddle = vCenter;
        const glm::dvec3 vBottom = vCenter - vBillboardUp * dSize;

        AppendLine(arrVertices, vLeftTop, vMiddle, vColor);
        AppendLine(arrVertices, vRightTop, vMiddle, vColor);
        AppendLine(arrVertices, vMiddle, vBottom, vColor);
    }

    void AppendGlyphZ(
        std::vector<SAxisMarkerVertex>& arrVertices,
        const glm::dvec3& vCenter,
        const glm::dvec3& vBillboardRight,
        const glm::dvec3& vBillboardUp,
        double dSize,
        const glm::vec4& vColor
    )
    {
        const glm::dvec3 vLeftTop = vCenter - vBillboardRight * dSize + vBillboardUp * dSize;
        const glm::dvec3 vRightTop = vCenter + vBillboardRight * dSize + vBillboardUp * dSize;
        const glm::dvec3 vLeftBottom = vCenter - vBillboardRight * dSize - vBillboardUp * dSize;
        const glm::dvec3 vRightBottom = vCenter + vBillboardRight * dSize - vBillboardUp * dSize;

        AppendLine(arrVertices, vLeftTop, vRightTop, vColor);
        AppendLine(arrVertices, vRightTop, vLeftBottom, vColor);
        AppendLine(arrVertices, vLeftBottom, vRightBottom, vColor);
    }

    void BuildAxisMarkerVertices(
        const SGridGeometry& sGridGeometry,
        const SAxisMarkerStyle& sStyle,
        const glm::dvec3& vCameraRight,
        const glm::dvec3& vCameraUp,
        std::vector<SAxisMarkerVertex>& arrVertices
    )
    {
        arrVertices.clear();

        const glm::dvec3 vOriginLocal(0.0, 0.0, 0.0);

        // –еальные направлени€ осей маркера в мире.
        const glm::dvec3 vAxisX = glm::normalize(sGridGeometry.vAxisX);
        const glm::dvec3 vAxisY = glm::normalize(sGridGeometry.vAxisY);
        const glm::dvec3 vAxisZ = glm::normalize(sGridGeometry.vNormal);

        const double dAxisLength = sStyle.dAxisLength;
        const double dLetterOffset = sStyle.dLetterOffset;
        const double dLetterSize = sStyle.dLetterSize;

        const glm::dvec3 vXEnd = vAxisX * dAxisLength;
        const glm::dvec3 vYEnd = vAxisY * dAxisLength;
        const glm::dvec3 vZEnd = vAxisZ * dAxisLength;

        //  ороткие оси из одной точки.
        AppendLine(arrVertices, vOriginLocal, vXEnd, sStyle.vAxisColor);
        AppendLine(arrVertices, vOriginLocal, vYEnd, sStyle.vAxisColor);
        AppendLine(arrVertices, vOriginLocal, vZEnd, sStyle.vAxisColor);

        // ÷ентры букв располагаем возле концов осей.
        const glm::dvec3 vXLabelCenter = vAxisX * (dAxisLength + dLetterOffset);
        const glm::dvec3 vYLabelCenter = vAxisY * (dAxisLength + dLetterOffset);
        const glm::dvec3 vZLabelCenter = vAxisZ * (dAxisLength + dLetterOffset);

        //  лючевой момент:
        // сами буквы строим не в базисе сетки, а в базисе камеры.
        // “огда X/Y/Z всегда "смотр€т" на нас.
        AppendGlyphX(
            arrVertices,
            vXLabelCenter,
            vCameraRight,
            vCameraUp,
            dLetterSize,
            sStyle.vTextColor
        );

        AppendGlyphY(
            arrVertices,
            vYLabelCenter,
            vCameraRight,
            vCameraUp,
            dLetterSize,
            sStyle.vTextColor
        );

        AppendGlyphZ(
            arrVertices,
            vZLabelCenter,
            vCameraRight,
            vCameraUp,
            dLetterSize,
            sStyle.vTextColor
        );
    }
}

CAxisMarkerRenderer::CAxisMarkerRenderer()
    : m_nVao(0)
    , m_nVbo(0)
{
    m_sStyle.dAxisLength = 2.0;
    m_sStyle.dLetterOffset = 0.45;
    m_sStyle.dLetterSize = 0.22;

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

    // location 0: dvec3 local position.
    glEnableVertexAttribArray(0);
    glVertexAttribLPointer(
        0,
        3,
        GL_DOUBLE,
        sizeof(SAxisMarkerVertex),
        reinterpret_cast<const void*>(offsetof(SAxisMarkerVertex, vLocalPosition))
    );

    // location 1: vec4 color.
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
    // »з view-матрицы получаем world-space базис камеры.
    //
    // inverse(view) содержит ориентацию камеры в мировом пространстве:
    // column 0 -> right
    // column 1 -> up
    const glm::dmat4 mInvView = glm::inverse(sFrameData.mView);

    const glm::dvec3 vCameraRight = glm::normalize(glm::dvec3(mInvView[0]));
    const glm::dvec3 vCameraUp = glm::normalize(glm::dvec3(mInvView[1]));

    std::vector<SAxisMarkerVertex> arrVertices;
    BuildAxisMarkerVertices(
        sGridGeometry,
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

    // ћаркер начала координат рисуем поверх сетки.
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