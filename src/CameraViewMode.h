#pragma once

// Режимы вида камеры.
//
// Важно:
// Top / Front / Right / IsometricOrthographic — это ортографические виды.
// Они отличаются ориентацией камеры, а не типом проекции.
//
// Perspective — отдельный тестовый режим с перспективной проекцией.
enum class ECameraViewMode
{
    Top,
    Front,
    Right,
    IsometricOrthographic,
    Perspective
};

// Параметры конкретного режима камеры.
struct SCameraViewModeSettings
{
    // Использовать ли ортографическую проекцию.
    bool bUseOrthographicProjection;

    // Yaw камеры в радианах.
    double dYawRadians;

    // Pitch камеры в радианах.
    double dPitchRadians;
};

// Возвращает настройки камеры для выбранного режима.
SCameraViewModeSettings GetCameraViewModeSettings(ECameraViewMode eMode);

// Возвращает имя режима для вывода в консоль.
const char* GetCameraViewModeName(ECameraViewMode eMode);
