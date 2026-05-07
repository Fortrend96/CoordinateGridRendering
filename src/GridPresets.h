#pragma once

#include "GridRenderer.h"

// Набор тестовых режимов сетки.
//
// Эти presets нужны только для демо-приложения:
// - проверить обычную сетку;
// - проверить большой сдвиг;
// - проверить поворот;
// - проверить большой сдвиг + поворот.
//
// Это не часть GridRenderer, а логика тестового стенда.
enum class EGridPreset
{
    Simple = 1,
    LargeOffset = 2,
    Rotated = 3,
    LargeOffsetAndRotated = 4
};

// Создаёт геометрию сетки для выбранного тестового режима.
//
// Возвращает:
// - origin;
// - ось X;
// - ось Y;
// - normal.
SGridGeometry CreateGridGeometry(EGridPreset ePreset);

// Возвращает текстовое имя preset'а для вывода в консоль.
const char* GetGridPresetName(EGridPreset ePreset);
