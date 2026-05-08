#include "CameraViewMode.h"

#include <glm/gtc/matrix_transform.hpp>

namespace
{
    // Почти вертикальный угол.
    //
    // Не используем ровно 90 градусов, потому что OrbitCamera может использовать
    // world up vector, и строго вертикальный взгляд иногда приводит к вырождению
    // camera basis.
    constexpr double dAlmostVerticalPitchDegrees = 89.9;

    // Классический изометрический угол.
    //
    // atan(1 / sqrt(2)) ~= 35.26438968 градусов.
    // Это стандартный угол для изометрического вида.
    constexpr double dIsometricPitchDegrees = 35.26438968;
}

SCameraViewModeSettings GetCameraViewModeSettings(ECameraViewMode eMode)
{
    switch (eMode)
    {
    case ECameraViewMode::Top:
        return SCameraViewModeSettings
        {
            true,
            glm::radians(-90.0),
            glm::radians(dAlmostVerticalPitchDegrees)
        };

    case ECameraViewMode::Front:
        return SCameraViewModeSettings
        {
            true,
            glm::radians(-90.0),
            glm::radians(0.0)
        };

    case ECameraViewMode::Right:
        return SCameraViewModeSettings
        {
            true,
            glm::radians(0.0),
            glm::radians(0.0)
        };

    case ECameraViewMode::IsometricOrthographic:
        return SCameraViewModeSettings
        {
            true,
            glm::radians(-45.0),
            glm::radians(dIsometricPitchDegrees)
        };

    case ECameraViewMode::Perspective:
        return SCameraViewModeSettings
        {
            false,
            glm::radians(-45.0),
            glm::radians(dIsometricPitchDegrees)
        };

    default:
        return SCameraViewModeSettings
        {
            true,
            glm::radians(0.0),
            glm::radians(dAlmostVerticalPitchDegrees)
        };
    }
}

const char* GetCameraViewModeName(ECameraViewMode eMode)
{
    switch (eMode)
    {
    case ECameraViewMode::Top:
        return "Top orthographic";

    case ECameraViewMode::Front:
        return "Front orthographic";

    case ECameraViewMode::Right:
        return "Right orthographic";

    case ECameraViewMode::IsometricOrthographic:
        return "Isometric orthographic";

    case ECameraViewMode::Perspective:
        return "Perspective";

    default:
        return "Unknown";
    }
}
