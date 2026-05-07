#include "GridPresets.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// —оздаЄт тестовую матрицу поворота сетки.
//
// ѕоворот нужен дл€ проверки того, что сетка работает не только
// в простой плоскости XY / Z = 0.
static glm::dmat4 CreateGridRotation(bool bRotated)
{
    if (!bRotated)
    {
        return glm::dmat4(1.0);
    }

    return
        glm::rotate(glm::dmat4(1.0), glm::radians(25.0), glm::dvec3(1.0, 0.0, 0.0)) *
        glm::rotate(glm::dmat4(1.0), glm::radians(15.0), glm::dvec3(0.0, 1.0, 0.0)) *
        glm::rotate(glm::dmat4(1.0), glm::radians(10.0), glm::dvec3(0.0, 0.0, 1.0));
}

SGridGeometry CreateGridGeometry(EGridPreset ePreset)
{
    const bool bLargeOffset =
        ePreset == EGridPreset::LargeOffset ||
        ePreset == EGridPreset::LargeOffsetAndRotated;

    const bool bRotated =
        ePreset == EGridPreset::Rotated ||
        ePreset == EGridPreset::LargeOffsetAndRotated;

    // Ѕольшой сдвиг нужен дл€ проверки точности вычислений.
    //
    // ¬ реальных CAD-сценах координаты могут быть далеко от начала мира,
    // и float-точности дл€ таких случаев часто недостаточно.
    const glm::dvec3 vOrigin = bLargeOffset
        ? glm::dvec3(1000000.0, -2000000.0, 500000.0)
        : glm::dvec3(0.0, 0.0, 0.0);

    const glm::dmat4 mGridRotation = CreateGridRotation(bRotated);

    // Ћокальна€ ось X сетки после поворота.
    const glm::dvec3 vAxisX = glm::normalize(
        glm::dvec3(mGridRotation * glm::dvec4(1.0, 0.0, 0.0, 0.0))
    );

    // Ћокальна€ ось Y сетки после поворота.
    const glm::dvec3 vAxisY = glm::normalize(
        glm::dvec3(mGridRotation * glm::dvec4(0.0, 1.0, 0.0, 0.0))
    );

    // Ќормаль к плоскости сетки.
    //
    // ѕока считаем, что оси X/Y ортонормированы.
    const glm::dvec3 vNormal = glm::normalize(glm::cross(vAxisX, vAxisY));

    SGridGeometry sGeometry;
    sGeometry.vOrigin = vOrigin;
    sGeometry.vAxisX = vAxisX;
    sGeometry.vAxisY = vAxisY;
    sGeometry.vNormal = vNormal;

    return sGeometry;
}

const char* GetGridPresetName(EGridPreset ePreset)
{
    switch (ePreset)
    {
    case EGridPreset::Simple:
        return "Simple";

    case EGridPreset::LargeOffset:
        return "Large offset";

    case EGridPreset::Rotated:
        return "Rotated";

    case EGridPreset::LargeOffsetAndRotated:
        return "Large offset + rotated";

    default:
        return "Unknown";
    }
}