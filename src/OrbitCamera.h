#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Орбитальная камера.
//
// Камера всегда смотрит на точку m_vTarget.
// Положение камеры задаётся тремя параметрами:
// - расстояние до цели;
// - угол вокруг вертикальной оси;
// - угол подъёма/наклона.
//
// Это удобно для проверки сетки:
// можно вращаться вокруг неё и смотреть, как она ведёт себя под разными углами.
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

    glm::dvec3 GetPosition() const;
    glm::dmat4 GetViewMatrix() const;

    void BeginRotation(double dMouseX, double dMouseY);
    void UpdateRotation(double dMouseX, double dMouseY);
    void EndRotation();

    void AddZoom(double dScrollOffset);

private:
    void ClampPitch();
    void ClampDistance();

private:
    glm::dvec3 m_vTarget;

    double m_dDistance;
    double m_dYawRadians;
    double m_dPitchRadians;

    bool m_bIsRotating;

    double m_dLastMouseX;
    double m_dLastMouseY;
};
