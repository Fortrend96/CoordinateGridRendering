#include "AxisMarkerRenderer.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>

namespace
{
    // Ось маркера.
    enum class EAxisMarkerAxis
    {
        X,
        Y,
        Z
    };

    // Вершина маркера.
    //
    // Позиция хранится в локальных координатах относительно origin сетки.
    // В shader'е world position получается как:
    //
    //     world = uGridOrigin + aLocalPosition
    //
    // Это важно для больших координат: origin может быть очень большим,
    // а сам маркер остаётся маленьким локальным объектом.
    struct SAxisMarkerVertex
    {
        // Локальная позиция вершины относительно origin сетки.
        glm::dvec3 vLocalPosition;

        // Цвет вершины.
        glm::vec4 vColor;
    };

    // Добавляет одну линию в массив вершин.
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

    // Добавляет квадрат в origin.
    //
    // Квадрат строится в billboard-базисе камеры,
    // поэтому в orthographic-режиме он выглядит как экранный квадрат,
    // как в AutoCAD UCS icon.
    void AppendOriginSquare(
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

        const glm::dvec3 vRightBottom =
            vCenter + vBillboardRight * dHalfSizeWorld - vBillboardUp * dHalfSizeWorld;

        const glm::dvec3 vRightTop =
            vCenter + vBillboardRight * dHalfSizeWorld + vBillboardUp * dHalfSizeWorld;

        const glm::dvec3 vLeftTop =
            vCenter - vBillboardRight * dHalfSizeWorld + vBillboardUp * dHalfSizeWorld;

        AppendLine(arrVertices, vLeftBottom, vRightBottom, vColor);
        AppendLine(arrVertices, vRightBottom, vRightTop, vColor);
        AppendLine(arrVertices, vRightTop, vLeftTop, vColor);
        AppendLine(arrVertices, vLeftTop, vLeftBottom, vColor);
    }

    // Добавляет букву X из двух линий.
    //
    // Буква строится в billboard-базисе камеры:
    // vBillboardRight/vBillboardUp задают экранную ориентацию символа.
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

    // Добавляет букву Y из трёх линий.
    //
    // Буква строится в billboard-базисе камеры.
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

    // Добавляет букву Z из трёх линий.
    //
    // Буква строится в billboard-базисе камеры.
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

    // Добавляет букву по идентификатору оси.
    void AppendAxisGlyph(
        std::vector<SAxisMarkerVertex>& arrVertices,
        EAxisMarkerAxis eAxis,
        const glm::dvec3& vCenter,
        const glm::dvec3& vBillboardRight,
        const glm::dvec3& vBillboardUp,
        double dHalfSizeWorld,
        const glm::vec4& vColor
    )
    {
        switch (eAxis)
        {
        case EAxisMarkerAxis::X:
            AppendGlyphX(
                arrVertices,
                vCenter,
                vBillboardRight,
                vBillboardUp,
                dHalfSizeWorld,
                vColor
            );
            break;

        case EAxisMarkerAxis::Y:
            AppendGlyphY(
                arrVertices,
                vCenter,
                vBillboardRight,
                vBillboardUp,
                dHalfSizeWorld,
                vColor
            );
            break;

        case EAxisMarkerAxis::Z:
            AppendGlyphZ(
                arrVertices,
                vCenter,
                vBillboardRight,
                vBillboardUp,
                dHalfSizeWorld,
                vColor
            );
            break;
        }
    }

    // Проецирует локальную точку маркера в screen-space.
    //
    // Локальная точка переводится в world-space через:
    //
    //     world = gridOrigin + local
    //
    // Затем world-space точка проецируется в пиксели framebuffer'а.
    bool ProjectLocalPointToScreen(
        const glm::dmat4& mViewProj,
        const glm::dvec3& vGridOrigin,
        const glm::dvec3& vLocalPosition,
        const glm::dvec2& vViewportSize,
        glm::dvec2& vScreenPosition
    )
    {
        const glm::dvec3 vWorldPosition = vGridOrigin + vLocalPosition;
        const glm::dvec4 vClipPosition = mViewProj * glm::dvec4(vWorldPosition, 1.0);

        if (std::abs(vClipPosition.w) < 1e-12)
        {
            return false;
        }

        const glm::dvec3 vNdc = glm::dvec3(vClipPosition) / vClipPosition.w;

        vScreenPosition = glm::dvec2(
            (vNdc.x * 0.5 + 0.5) * vViewportSize.x,
            (vNdc.y * 0.5 + 0.5) * vViewportSize.y
        );

        return true;
    }

    // Считает, сколько пикселей занимает 1 world unit
    // в заданном локальном направлении около origin маркера.
    double GetPixelsPerWorldUnit(
        const glm::dmat4& mViewProj,
        const glm::dvec3& vGridOrigin,
        const glm::dvec3& vLocalDirection,
        const glm::dvec2& vViewportSize
    )
    {
        const glm::dvec3 vDirection = glm::normalize(vLocalDirection);

        glm::dvec2 vScreenOrigin(0.0);
        glm::dvec2 vScreenUnit(0.0);

        const bool bOriginProjected = ProjectLocalPointToScreen(
            mViewProj,
            vGridOrigin,
            glm::dvec3(0.0, 0.0, 0.0),
            vViewportSize,
            vScreenOrigin
        );

        const bool bUnitProjected = ProjectLocalPointToScreen(
            mViewProj,
            vGridOrigin,
            vDirection,
            vViewportSize,
            vScreenUnit
        );

        if (!bOriginProjected || !bUnitProjected)
        {
            return 1.0;
        }

        return std::max(glm::length(vScreenUnit - vScreenOrigin), 1e-6);
    }

    // Переводит размер в пикселях в world units
    // по заранее рассчитанному pixels-per-world-unit.
    double PixelsToWorldUnits(
        double dPixels,
        double dPixelsPerWorldUnit
    )
    {
        return dPixels / std::max(dPixelsPerWorldUnit, 1e-6);
    }

    // Возвращает world-space right/up/viewDirection камеры из обратной view-матрицы.
    //
    // Для OpenGL-камеры:
    // - local X камеры — right;
    // - local Y камеры — up;
    // - local -Z камеры — view direction.
    void ExtractCameraBasis(
        const glm::dmat4& mView,
        glm::dvec3& vCameraRight,
        glm::dvec3& vCameraUp,
        glm::dvec3& vCameraViewDirection
    )
    {
        const glm::dmat4 mInvView = glm::inverse(mView);

        vCameraRight = glm::normalize(glm::dvec3(mInvView[0]));
        vCameraUp = glm::normalize(glm::dvec3(mInvView[1]));

        // В OpenGL камера смотрит вдоль локальной -Z.
        vCameraViewDirection = glm::normalize(-glm::dvec3(mInvView[2]));
    }

    // Возвращает локальное направление оси маркера.
    glm::dvec3 GetAxisDirection(
        const SGridGeometry& sGridGeometry,
        EAxisMarkerAxis eAxis
    )
    {
        switch (eAxis)
        {
        case EAxisMarkerAxis::X:
            return glm::normalize(sGridGeometry.vAxisX);

        case EAxisMarkerAxis::Y:
            return glm::normalize(sGridGeometry.vAxisY);

        case EAxisMarkerAxis::Z:
            return glm::normalize(sGridGeometry.vNormal);

        default:
            return glm::dvec3(1.0, 0.0, 0.0);
        }
    }

    // Возвращает экранную длину проекции оси маркера.
    //
    // Используется для определения:
    // - какая ось почти направлена в камеру;
    // - похож ли текущий orthographic view на один из классических CAD-видов.
    double GetAxisScreenLength(
        const SGridFrameData& sFrameData,
        const SGridGeometry& sGridGeometry,
        EAxisMarkerAxis eAxis
    )
    {
        const glm::dvec3 vAxisDirection = GetAxisDirection(
            sGridGeometry,
            eAxis
        );

        glm::dvec2 vScreenOrigin(0.0);
        glm::dvec2 vScreenAxisPoint(0.0);

        const bool bOriginProjected = ProjectLocalPointToScreen(
            sFrameData.mViewProj,
            sGridGeometry.vOrigin,
            glm::dvec3(0.0, 0.0, 0.0),
            sFrameData.vViewportSize,
            vScreenOrigin
        );

        const bool bAxisProjected = ProjectLocalPointToScreen(
            sFrameData.mViewProj,
            sGridGeometry.vOrigin,
            vAxisDirection,
            sFrameData.vViewportSize,
            vScreenAxisPoint
        );

        if (!bOriginProjected || !bAxisProjected)
        {
            return 0.0;
        }

        return glm::length(vScreenAxisPoint - vScreenOrigin);
    }

    // Проверяет, похож ли текущий orthographic view на один из классических
    // осевых CAD-видов: Top/Bottom, Front/Back, Left/Right.
    //
    // Важно:
    // orthographic projection сама по себе НЕ означает, что надо скрывать одну ось.
    // Пользователь может вращать камеру в ортографической проекции.
    // В таком случае нужно рисовать обычный 3D-маркер, иначе одна из осей будет
    // внезапно исчезать при вращении камеры.
    //
    // Поэтому плоский AutoCAD-like UCS marker включаем только тогда,
    // когда одна из осей почти направлена в камеру.
    bool IsPrincipalOrthographicView(
        const SGridFrameData& sFrameData,
        const SGridGeometry& sGridGeometry
    )
    {
        const double dScreenLengthX = GetAxisScreenLength(
            sFrameData,
            sGridGeometry,
            EAxisMarkerAxis::X
        );

        const double dScreenLengthY = GetAxisScreenLength(
            sFrameData,
            sGridGeometry,
            EAxisMarkerAxis::Y
        );

        const double dScreenLengthZ = GetAxisScreenLength(
            sFrameData,
            sGridGeometry,
            EAxisMarkerAxis::Z
        );

        const double dMinLength = std::min(
            dScreenLengthX,
            std::min(dScreenLengthY, dScreenLengthZ)
        );

        const double dMaxLength = std::max(
            dScreenLengthX,
            std::max(dScreenLengthY, dScreenLengthZ)
        );

        if (dMaxLength <= 1e-6)
        {
            return false;
        }

        // Если одна из осей спроецировалась почти в точку,
        // значит камера смотрит почти вдоль этой оси.
        //
        // 0.08 означает: минимальная экранная длина оси меньше 8%
        // от самой длинной экранной оси.
        //
        // Это достаточно строго, чтобы плоский UCS marker включался только
        // в почти осевых видах, а не при произвольном вращении камеры.
        const double dPrincipalViewThreshold = 0.08;

        return (dMinLength / dMaxLength) < dPrincipalViewThreshold;
    }

    // Определяет, какую ось скрыть в orthographic-режиме,
    // по экранной длине её проекции.
    //
    // Эта функция вызывается только для principal orthographic view.
    //
    // Логика:
    // - проецируем X/Y/Z;
    // - ось с минимальной экранной длиной почти направлена в камеру;
    // - её скрываем.
    EAxisMarkerAxis ChooseHiddenAxisForOrthographicView(
        const SGridFrameData& sFrameData,
        const SGridGeometry& sGridGeometry
    )
    {
        const double dScreenLengthX = GetAxisScreenLength(
            sFrameData,
            sGridGeometry,
            EAxisMarkerAxis::X
        );

        const double dScreenLengthY = GetAxisScreenLength(
            sFrameData,
            sGridGeometry,
            EAxisMarkerAxis::Y
        );

        const double dScreenLengthZ = GetAxisScreenLength(
            sFrameData,
            sGridGeometry,
            EAxisMarkerAxis::Z
        );

        if (dScreenLengthX <= dScreenLengthY && dScreenLengthX <= dScreenLengthZ)
        {
            return EAxisMarkerAxis::X;
        }

        if (dScreenLengthY <= dScreenLengthX && dScreenLengthY <= dScreenLengthZ)
        {
            return EAxisMarkerAxis::Y;
        }

        return EAxisMarkerAxis::Z;
    }

    // Проецирует ось маркера в screen-space и возвращает экранное направление.
    //
    // Это нужно для orthographic-маркера:
    // линия оси должна идти ровно в том направлении, в котором ось видна на экране,
    // но длина линии должна быть фиксированной в пикселях.
    bool TryGetAxisScreenDirection(
        const SGridFrameData& sFrameData,
        const SGridGeometry& sGridGeometry,
        const glm::dvec3& vAxisDirection,
        glm::dvec2& vScreenDirection
    )
    {
        glm::dvec2 vScreenOrigin(0.0);
        glm::dvec2 vScreenAxisPoint(0.0);

        const bool bOriginProjected = ProjectLocalPointToScreen(
            sFrameData.mViewProj,
            sGridGeometry.vOrigin,
            glm::dvec3(0.0, 0.0, 0.0),
            sFrameData.vViewportSize,
            vScreenOrigin
        );

        const bool bAxisProjected = ProjectLocalPointToScreen(
            sFrameData.mViewProj,
            sGridGeometry.vOrigin,
            vAxisDirection,
            sFrameData.vViewportSize,
            vScreenAxisPoint
        );

        if (!bOriginProjected || !bAxisProjected)
        {
            return false;
        }

        const glm::dvec2 vDelta = vScreenAxisPoint - vScreenOrigin;
        const double dLength = glm::length(vDelta);

        if (dLength < 1e-6)
        {
            return false;
        }

        vScreenDirection = vDelta / dLength;
        return true;
    }

    // Строит одну экранно-стабильную ось для orthographic-маркера.
    //
    // Экранное направление берётся из проекции реальной оси,
    // а затем переводится обратно в world-space через cameraRight/cameraUp.
    // Благодаря этому длина оси остаётся фиксированной в пикселях.
    void AppendOrthographicAxis(
        std::vector<SAxisMarkerVertex>& arrVertices,
        const SGridFrameData& sFrameData,
        const SGridGeometry& sGridGeometry,
        const SAxisMarkerStyle& sStyle,
        EAxisMarkerAxis eAxis,
        const glm::dvec3& vCameraRight,
        const glm::dvec3& vCameraUp,
        double dPixelsPerBillboardWorldUnit
    )
    {
        const glm::dvec3 vAxisDirection = GetAxisDirection(sGridGeometry, eAxis);

        glm::dvec2 vScreenDirection(0.0);

        const bool bHasScreenDirection = TryGetAxisScreenDirection(
            sFrameData,
            sGridGeometry,
            vAxisDirection,
            vScreenDirection
        );

        if (!bHasScreenDirection)
        {
            return;
        }

        const double dAxisLengthWorld = PixelsToWorldUnits(
            sStyle.dAxisLengthPixels,
            dPixelsPerBillboardWorldUnit
        );

        const double dLetterOffsetWorld = PixelsToWorldUnits(
            sStyle.dLetterOffsetPixels,
            dPixelsPerBillboardWorldUnit
        );

        const double dLetterHalfSizeWorld = PixelsToWorldUnits(
            sStyle.dLetterSizePixels,
            dPixelsPerBillboardWorldUnit
        );

        // Переводим экранное направление в world-space направление.
        //
        // vScreenDirection.x идёт вдоль cameraRight,
        // vScreenDirection.y идёт вдоль cameraUp.
        const glm::dvec3 vWorldScreenDirection = glm::normalize(
            vCameraRight * vScreenDirection.x +
            vCameraUp * vScreenDirection.y
        );

        const glm::dvec3 vBegin(0.0, 0.0, 0.0);
        const glm::dvec3 vEnd = vWorldScreenDirection * dAxisLengthWorld;

        AppendLine(
            arrVertices,
            vBegin,
            vEnd,
            sStyle.vAxisColor
        );

        const glm::dvec3 vLabelCenter =
            vWorldScreenDirection * (dAxisLengthWorld + dLetterOffsetWorld);

        AppendAxisGlyph(
            arrVertices,
            eAxis,
            vLabelCenter,
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
    // Все размеры задаются в пикселях,
    // чтобы маркер не менял визуальный размер при zoom.
    m_sStyle.dAxisLengthPixels = 48.0;
    m_sStyle.dLetterOffsetPixels = 10.0;
    m_sStyle.dLetterSizePixels = 6.0;
    m_sStyle.dOriginSquareHalfSizePixels = 4.0;

    m_sStyle.fLineWidth = 1.5f;

    m_sStyle.vAxisColor = glm::vec4(0.92f, 0.92f, 0.92f, 1.0f);
    m_sStyle.vTextColor = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
    m_sStyle.vOriginSquareColor = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
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
    // Бесшовная логика projection modes.
    //
    // Perspective:
    //   всегда рисуем 3D-маркер X/Y/Z.
    //
    // Orthographic:
    //   если камера почти в одном из классических CAD-видов
    //   Top/Bottom/Front/Back/Left/Right, рисуем плоский UCS marker
    //   и скрываем ось, направленную в камеру.
    //
    //   если камера повернута произвольно, НЕ скрываем ось и рисуем обычный
    //   3D-маркер. Иначе при вращении в orthographic projection одна из осей
    //   будет внезапно пропадать.
    const bool bUseFlatOrthographicMarker =
        sFrameData.bIsOrthographicProjection &&
        IsPrincipalOrthographicView(
            sFrameData,
            sGridGeometry
        );

    if (bUseFlatOrthographicMarker)
    {
        RenderOrthographicMarker(
            shaderProgram,
            sFrameData,
            sGridGeometry
        );
    }
    else
    {
        RenderPerspectiveMarker(
            shaderProgram,
            sFrameData,
            sGridGeometry
        );
    }
}

void CAxisMarkerRenderer::RenderPerspectiveMarker(
    const CShaderProgram& shaderProgram,
    const SGridFrameData& sFrameData,
    const SGridGeometry& sGridGeometry
) const
{
    glm::dvec3 vCameraRight(1.0, 0.0, 0.0);
    glm::dvec3 vCameraUp(0.0, 1.0, 0.0);
    glm::dvec3 vCameraViewDirection(0.0, 0.0, -1.0);

    ExtractCameraBasis(
        sFrameData.mView,
        vCameraRight,
        vCameraUp,
        vCameraViewDirection
    );

    const glm::dvec3 vAxisX = GetAxisDirection(sGridGeometry, EAxisMarkerAxis::X);
    const glm::dvec3 vAxisY = GetAxisDirection(sGridGeometry, EAxisMarkerAxis::Y);
    const glm::dvec3 vAxisZ = GetAxisDirection(sGridGeometry, EAxisMarkerAxis::Z);

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
        m_sStyle.dAxisLengthPixels,
        dPixelsPerUnitX
    );

    const double dAxisLengthYWorld = PixelsToWorldUnits(
        m_sStyle.dAxisLengthPixels,
        dPixelsPerUnitY
    );

    const double dAxisLengthZWorld = PixelsToWorldUnits(
        m_sStyle.dAxisLengthPixels,
        dPixelsPerUnitZ
    );

    const double dLetterOffsetWorld = PixelsToWorldUnits(
        m_sStyle.dLetterOffsetPixels,
        dPixelsPerUnitBillboard
    );

    const double dLetterHalfSizeWorld = PixelsToWorldUnits(
        m_sStyle.dLetterSizePixels,
        dPixelsPerUnitBillboard
    );

    std::vector<SAxisMarkerVertex> arrVertices;

    const glm::dvec3 vOriginLocal(0.0, 0.0, 0.0);

    const glm::dvec3 vXEnd = vAxisX * dAxisLengthXWorld;
    const glm::dvec3 vYEnd = vAxisY * dAxisLengthYWorld;
    const glm::dvec3 vZEnd = vAxisZ * dAxisLengthZWorld;

    // Perspective-режим: показываем все три оси.
    AppendLine(arrVertices, vOriginLocal, vXEnd, m_sStyle.vAxisColor);
    AppendLine(arrVertices, vOriginLocal, vYEnd, m_sStyle.vAxisColor);
    AppendLine(arrVertices, vOriginLocal, vZEnd, m_sStyle.vAxisColor);

    AppendGlyphX(
        arrVertices,
        vAxisX * (dAxisLengthXWorld + dLetterOffsetWorld),
        vCameraRight,
        vCameraUp,
        dLetterHalfSizeWorld,
        m_sStyle.vTextColor
    );

    AppendGlyphY(
        arrVertices,
        vAxisY * (dAxisLengthYWorld + dLetterOffsetWorld),
        vCameraRight,
        vCameraUp,
        dLetterHalfSizeWorld,
        m_sStyle.vTextColor
    );

    AppendGlyphZ(
        arrVertices,
        vAxisZ * (dAxisLengthZWorld + dLetterOffsetWorld),
        vCameraRight,
        vCameraUp,
        dLetterHalfSizeWorld,
        m_sStyle.vTextColor
    );

    UploadAndDrawLines(
        shaderProgram,
        sFrameData,
        sGridGeometry,
        arrVertices.data(),
        static_cast<GLsizeiptr>(arrVertices.size() * sizeof(SAxisMarkerVertex)),
        static_cast<GLsizei>(arrVertices.size())
    );
}

void CAxisMarkerRenderer::RenderOrthographicMarker(
    const CShaderProgram& shaderProgram,
    const SGridFrameData& sFrameData,
    const SGridGeometry& sGridGeometry
) const
{
    glm::dvec3 vCameraRight(1.0, 0.0, 0.0);
    glm::dvec3 vCameraUp(0.0, 1.0, 0.0);
    glm::dvec3 vCameraViewDirection(0.0, 0.0, -1.0);

    ExtractCameraBasis(
        sFrameData.mView,
        vCameraRight,
        vCameraUp,
        vCameraViewDirection
    );

    // В orthographic principal view скрываем ту ось,
    // которая почти не имеет экранной длины.
    // Это соответствует AutoCAD-like UCS icon.
    const EAxisMarkerAxis eHiddenAxis = ChooseHiddenAxisForOrthographicView(
        sFrameData,
        sGridGeometry
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

    const double dOriginSquareHalfSizeWorld = PixelsToWorldUnits(
        m_sStyle.dOriginSquareHalfSizePixels,
        dPixelsPerUnitBillboard
    );

    std::vector<SAxisMarkerVertex> arrVertices;

    // Orthographic-режим: в origin рисуем маленький квадрат,
    // как UCS icon в AutoCAD.
    AppendOriginSquare(
        arrVertices,
        glm::dvec3(0.0, 0.0, 0.0),
        vCameraRight,
        vCameraUp,
        dOriginSquareHalfSizeWorld,
        m_sStyle.vOriginSquareColor
    );

    // Добавляем только те оси, которые не направлены в камеру.
    if (eHiddenAxis != EAxisMarkerAxis::X)
    {
        AppendOrthographicAxis(
            arrVertices,
            sFrameData,
            sGridGeometry,
            m_sStyle,
            EAxisMarkerAxis::X,
            vCameraRight,
            vCameraUp,
            dPixelsPerUnitBillboard
        );
    }

    if (eHiddenAxis != EAxisMarkerAxis::Y)
    {
        AppendOrthographicAxis(
            arrVertices,
            sFrameData,
            sGridGeometry,
            m_sStyle,
            EAxisMarkerAxis::Y,
            vCameraRight,
            vCameraUp,
            dPixelsPerUnitBillboard
        );
    }

    if (eHiddenAxis != EAxisMarkerAxis::Z)
    {
        AppendOrthographicAxis(
            arrVertices,
            sFrameData,
            sGridGeometry,
            m_sStyle,
            EAxisMarkerAxis::Z,
            vCameraRight,
            vCameraUp,
            dPixelsPerUnitBillboard
        );
    }

    UploadAndDrawLines(
        shaderProgram,
        sFrameData,
        sGridGeometry,
        arrVertices.data(),
        static_cast<GLsizeiptr>(arrVertices.size() * sizeof(SAxisMarkerVertex)),
        static_cast<GLsizei>(arrVertices.size())
    );
}

void CAxisMarkerRenderer::UploadAndDrawLines(
    const CShaderProgram& shaderProgram,
    const SGridFrameData& sFrameData,
    const SGridGeometry& sGridGeometry,
    const void* pVertexData,
    GLsizeiptr nVertexDataSize,
    GLsizei nVertexCount
) const
{
    if (pVertexData == nullptr || nVertexDataSize <= 0 || nVertexCount <= 0)
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
        nVertexDataSize,
        pVertexData,
        GL_DYNAMIC_DRAW
    );

    // Маркер рисуем поверх сетки, как вспомогательный viewport UI.
    const GLboolean bDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);

    GLfloat fOldLineWidth = 1.0f;
    glGetFloatv(GL_LINE_WIDTH, &fOldLineWidth);

    if (bDepthTestEnabled == GL_TRUE)
    {
        glDisable(GL_DEPTH_TEST);
    }

    glLineWidth(m_sStyle.fLineWidth);

    glDrawArrays(GL_LINES, 0, nVertexCount);

    glLineWidth(fOldLineWidth);

    if (bDepthTestEnabled == GL_TRUE)
    {
        glEnable(GL_DEPTH_TEST);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}