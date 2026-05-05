#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class ECameraDragMode
{
    None,
    Orbit,
    Pan
};

// Орбитальная камера.
//
// Камера всегда смотрит на точку m_vTarget.
// Положение камеры задаётся:
// - точкой цели;
// - расстоянием до цели;
// - yaw;
// - pitch.
//
// Управление приближено к CAD-навигации:
// - orbit вращает камеру вокруг target;
// - pan сдвигает target в плоскости экрана;
// - zoom меняет distance.
class COrbitCamera
{
public:
    COrbitCamera();

    COrbitCamera(
        const glm::dvec3& vTarget,
        double dDistance,
        double dYawRadians,
        double dPitchRadians
    );

    void Reset(
        const glm::dvec3& vTarget,
        double dDistance,
        double dYawRadians,
        double dPitchRadians
    );

    void SetTarget(const glm::dvec3& vTarget);
    const glm::dvec3& GetTarget() const;

    double GetDistance() const;

    glm::dvec3 GetPosition() const;
    glm::dmat4 GetViewMatrix() const;

    void BeginOrbit(double dMouseX, double dMouseY);
    void BeginPan(double dMouseX, double dMouseY);

    void UpdateDrag(double dMouseX, double dMouseY);
    void EndDrag();

    void AddZoom(double dScrollOffset);

private:
    glm::dvec3 GetForwardDirection() const;
    glm::dvec3 GetRightDirection() const;
    glm::dvec3 GetUpDirection() const;

    void ClampPitch();
    void ClampDistance();

private:
    glm::dvec3 m_vTarget;

    double m_dDistance;
    double m_dYawRadians;
    double m_dPitchRadians;

    ECameraDragMode m_eDragMode;

    double m_dLastMouseX;
    double m_dLastMouseY;
};