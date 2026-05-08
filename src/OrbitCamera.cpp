#include "OrbitCamera.h"

#include <algorithm>
#include <cmath>

COrbitCamera::COrbitCamera()
    : m_vTarget(0.0, 0.0, 0.0)
    , m_dDistance(20.0)
    , m_dYawRadians(glm::radians(-45.0))
    , m_dPitchRadians(glm::radians(30.0))
    , m_eDragMode(ECameraDragMode::None)
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
    , m_eDragMode(ECameraDragMode::None)
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

    m_eDragMode = ECameraDragMode::None;
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

double COrbitCamera::GetDistance() const
{
    return m_dDistance;
}

glm::dvec3 COrbitCamera::GetForwardDirection() const
{
    // Ќаправление от target к камере.
    const double dCosPitch = std::cos(m_dPitchRadians);
    const double dSinPitch = std::sin(m_dPitchRadians);

    const double dCosYaw = std::cos(m_dYawRadians);
    const double dSinYaw = std::sin(m_dYawRadians);

    return glm::normalize(glm::dvec3(
        dCosPitch * dCosYaw,
        dCosPitch * dSinYaw,
        dSinPitch
    ));
}

glm::dvec3 COrbitCamera::GetPosition() const
{
    return m_vTarget + GetForwardDirection() * m_dDistance;
}

glm::dmat4 COrbitCamera::GetViewMatrix() const
{
    const glm::dvec3 vCameraPosition = GetPosition();
    const glm::dvec3 vWorldUp(0.0, 0.0, 1.0);

    return glm::lookAt(vCameraPosition, m_vTarget, vWorldUp);
}

glm::dvec3 COrbitCamera::GetRightDirection() const
{
    const glm::dvec3 vWorldUp(0.0, 0.0, 1.0);

    // View direction Ч от камеры к target.
    const glm::dvec3 vViewDirection = glm::normalize(m_vTarget - GetPosition());

    return glm::normalize(glm::cross(vViewDirection, vWorldUp));
}

glm::dvec3 COrbitCamera::GetUpDirection() const
{
    const glm::dvec3 vViewDirection = glm::normalize(m_vTarget - GetPosition());
    const glm::dvec3 vRight = GetRightDirection();

    return glm::normalize(glm::cross(vRight, vViewDirection));
}

void COrbitCamera::BeginOrbit(double dMouseX, double dMouseY)
{
    m_eDragMode = ECameraDragMode::Orbit;
    m_dLastMouseX = dMouseX;
    m_dLastMouseY = dMouseY;
}

void COrbitCamera::BeginPan(double dMouseX, double dMouseY)
{
    m_eDragMode = ECameraDragMode::Pan;
    m_dLastMouseX = dMouseX;
    m_dLastMouseY = dMouseY;
}

void COrbitCamera::UpdateDrag(double dMouseX, double dMouseY)
{
    if (m_eDragMode == ECameraDragMode::None)
    {
        return;
    }

    const double dDeltaX = dMouseX - m_dLastMouseX;
    const double dDeltaY = dMouseY - m_dLastMouseY;

    m_dLastMouseX = dMouseX;
    m_dLastMouseY = dMouseY;

    if (m_eDragMode == ECameraDragMode::Orbit)
    {
        const double dRotationSensitivity = 0.005;

        m_dYawRadians -= dDeltaX * dRotationSensitivity;
        m_dPitchRadians += dDeltaY * dRotationSensitivity;

        ClampPitch();
    }
    else if (m_eDragMode == ECameraDragMode::Pan)
    {
        const double dPanSensitivity = 0.0015 * m_dDistance;

        const glm::dvec3 vRight = GetRightDirection();
        const glm::dvec3 vUp = GetUpDirection();

        // «нак подобран так, чтобы движение мыши ощущалось как "перетаскивание чертежа".
        m_vTarget -= vRight * dDeltaX * dPanSensitivity;
        m_vTarget += vUp * dDeltaY * dPanSensitivity;
    }
}

void COrbitCamera::EndDrag()
{
    m_eDragMode = ECameraDragMode::None;
}

void COrbitCamera::AddZoom(double dScrollOffset)
{
    const double dZoomFactor = std::pow(0.90, dScrollOffset);

    m_dDistance *= dZoomFactor;

    ClampDistance();
}

void COrbitCamera::ClampPitch()
{
    // Ќе даЄм камере попасть ровно в вертикальное направление,
    // чтобы glm::lookAt не получил вырожденный up-vector.
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