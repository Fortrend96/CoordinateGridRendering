#include "OrbitCamera.h"

#include <algorithm>
#include <cmath>

COrbitCamera::COrbitCamera()
    : m_vTarget(0.0, 0.0, 0.0)
    , m_dDistance(20.0)
    , m_dYawRadians(glm::radians(-45.0))
    , m_dPitchRadians(glm::radians(30.0))
    , m_bIsRotating(false)
    , m_dLastMouseX(0.0)
    , m_dLastMouseY(0.0)
{
}

COrbitCamera::COrbitCamera(
    const glm::dvec3& vTarget,
    double dDistance,
    double dYawRadians,
    double dPitchRadians
)
    : m_vTarget(vTarget)
    , m_dDistance(dDistance)
    , m_dYawRadians(dYawRadians)
    , m_dPitchRadians(dPitchRadians)
    , m_bIsRotating(false)
    , m_dLastMouseX(0.0)
    , m_dLastMouseY(0.0)
{
    ClampPitch();
    ClampDistance();
}

void COrbitCamera::Reset(
    const glm::dvec3& vTarget,
    double dDistance,
    double dYawRadians,
    double dPitchRadians
)
{
    m_vTarget = vTarget;
    m_dDistance = dDistance;
    m_dYawRadians = dYawRadians;
    m_dPitchRadians = dPitchRadians;

    m_bIsRotating = false;
    m_dLastMouseX = 0.0;
    m_dLastMouseY = 0.0;

    ClampPitch();
    ClampDistance();
}

void COrbitCamera::SetTarget(const glm::dvec3& vTarget)
{
    m_vTarget = vTarget;
}

const glm::dvec3& COrbitCamera::GetTarget() const
{
    return m_vTarget;
}

glm::dvec3 COrbitCamera::GetPosition() const
{
    // —ферические координаты вокруг m_vTarget.
    //
    // m_dYawRadians   Ч поворот вокруг вертикальной оси Z.
    // m_dPitchRadians Ч угол подъЄма камеры.
    const double dCosPitch = std::cos(m_dPitchRadians);
    const double dSinPitch = std::sin(m_dPitchRadians);

    const double dCosYaw = std::cos(m_dYawRadians);
    const double dSinYaw = std::sin(m_dYawRadians);

    const glm::dvec3 vDirection(
        dCosPitch * dCosYaw,
        dCosPitch * dSinYaw,
        dSinPitch
    );

    return m_vTarget + vDirection * m_dDistance;
}

glm::dmat4 COrbitCamera::GetViewMatrix() const
{
    const glm::dvec3 vCameraPosition = GetPosition();

    // ƒл€ CAD-подобного мира считаем, что Z Ч вертикальна€ ось.
    const glm::dvec3 vUp(0.0, 0.0, 1.0);

    return glm::lookAt(vCameraPosition, m_vTarget, vUp);
}

void COrbitCamera::BeginRotation(double dMouseX, double dMouseY)
{
    m_bIsRotating = true;
    m_dLastMouseX = dMouseX;
    m_dLastMouseY = dMouseY;
}

void COrbitCamera::UpdateRotation(double dMouseX, double dMouseY)
{
    if (!m_bIsRotating)
    {
        return;
    }

    const double dDeltaX = dMouseX - m_dLastMouseX;
    const double dDeltaY = dMouseY - m_dLastMouseY;

    m_dLastMouseX = dMouseX;
    m_dLastMouseY = dMouseY;

    // „увствительность вращени€ мышью.
    const double dRotationSensitivity = 0.005;

    m_dYawRadians -= dDeltaX * dRotationSensitivity;
    m_dPitchRadians += dDeltaY * dRotationSensitivity;

    ClampPitch();
}

void COrbitCamera::EndRotation()
{
    m_bIsRotating = false;
}

void COrbitCamera::AddZoom(double dScrollOffset)
{
    // ѕоложительный scroll приближает камеру,
    // отрицательный Ч отдал€ет.
    //
    // ћультипликативный zoom обычно ощущаетс€ лучше,
    // чем простое вычитание фиксированной величины.
    const double dZoomFactor = std::pow(0.90, dScrollOffset);

    m_dDistance *= dZoomFactor;

    ClampDistance();
}

void COrbitCamera::ClampPitch()
{
    // Ќе даЄм камере попасть ровно в вертикальное направление,
    // чтобы glm::lookAt не получил вырожденный up-vector случай.
    const double dMaxPitch = glm::radians(89.0);

    m_dPitchRadians = std::clamp(
        m_dPitchRadians,
        -dMaxPitch,
        dMaxPitch
    );
}

void COrbitCamera::ClampDistance()
{
    m_dDistance = std::clamp(m_dDistance, 0.5, 1000000.0);
}
