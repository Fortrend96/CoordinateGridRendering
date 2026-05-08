#version 430 core

layout(location = 0) out vec4 outColor;

// -----------------------------------------------------------------------------
// Данные луча из vertex shader
// -----------------------------------------------------------------------------

// Локальная точка луча на near plane относительно uGridOrigin.
noperspective in vec3 vNearLocal;

// Локальная точка луча на far plane относительно uGridOrigin.
noperspective in vec3 vFarLocal;

// -----------------------------------------------------------------------------
// Матрицы
// -----------------------------------------------------------------------------

// View-projection матрица.
// Нужна для вычисления настоящей глубины фрагмента сетки.
uniform dmat4 uViewProj;

// -----------------------------------------------------------------------------
// Геометрия сетки
// -----------------------------------------------------------------------------

// Начало сетки в world-space.
uniform dvec3 uGridOrigin;

// Ось X сетки в world-space.
// Ожидается, что нормализована.
uniform dvec3 uGridAxisX;

// Ось Y сетки в world-space.
// Ожидается, что нормализована.
uniform dvec3 uGridAxisY;

// Нормаль плоскости сетки в world-space.
// Ожидается, что нормализована.
uniform dvec3 uGridNormal;

// Минимально допустимый abs(dot(rayDir, normal)).
//
// Если значение меньше, значит мы смотрим почти вдоль плоскости.
// В таком случае сетка может выглядеть коряво, поэтому гасим её заранее.
uniform double uMinViewNormalDot;

// -----------------------------------------------------------------------------
// Режимы отображения
// -----------------------------------------------------------------------------

// Ручной clamp глубины.
// Если true, depth прижимается к безопасному диапазону.
// Если false, фрагменты вне depth range отбрасываются.
uniform bool uClampDepth;

// Безопасный отступ от границ depth buffer.
// Вместо clamp(depth, 0.0, 1.0) используем:
// clamp(depth, epsilon, 1.0 - epsilon).
uniform double uSafeDepthEpsilon;

// Отладочный режим зон глубины.
// В этом режиме линии/точки сетки не рисуются,
// а плоскость красится по зонам:
// near / normal / far.
uniform bool uDebugDepthZones;

// false — рисуем только линии/оси;
// true  — дополнительно рисуем заливку плоскости сетки.
uniform bool uDrawPlane;

// false — сетка бесконечная;
// true  — сетка ограничена прямоугольником uGridBounds.
uniform bool uIsBounded;

// Границы bounded-сетки в локальной СК:
// x = minX,
// y = minY,
// z = maxX,
// w = maxY.
uniform dvec4 uGridBounds;

// false — рисуем линии;
// true  — рисуем точки в узлах сетки.
uniform bool uDrawDots;

// Показывать малую сетку.
uniform bool uShowMinorGrid;

// Показывать большую сетку.
uniform bool uShowMajorGrid;

// Показывать оси X/Y.
uniform bool uShowAxes;

// -----------------------------------------------------------------------------
// Шаги и толщины
// -----------------------------------------------------------------------------

// Малый шаг сетки.
uniform double uMinorStep;

// Большой шаг сетки.
uniform double uMajorStep;

// Толщина малых линий.
uniform float uMinorThickness;

// Толщина больших линий.
uniform float uMajorThickness;

// Толщина осей.
uniform float uAxisThickness;

// Радиус точек в dots-режиме.
uniform float uDotRadius;

// -----------------------------------------------------------------------------
// Цвета верхней стороны
// -----------------------------------------------------------------------------

uniform vec4 uPlaneColorTop;
uniform vec4 uMinorColorTop;
uniform vec4 uMajorColorTop;
uniform vec4 uXAxisColorTop;
uniform vec4 uYAxisColorTop;

// -----------------------------------------------------------------------------
// Цвета нижней стороны
// -----------------------------------------------------------------------------

uniform vec4 uPlaneColorBottom;
uniform vec4 uMinorColorBottom;
uniform vec4 uMajorColorBottom;
uniform vec4 uXAxisColorBottom;
uniform vec4 uYAxisColorBottom;

// Расстояние от координаты x до ближайшей линии, кратной step.
double DistanceToGridLine(double x, double step)
{
    double nearestLine = round(x / step) * step;
    return abs(x - nearestLine);
}

// Маска линии с антиалиасингом через fwidth.
float CreateLineMask(double distanceToLine, float pixelWidth, float thickness)
{
    return 1.0 - smoothstep(
        0.0,
        pixelWidth * thickness,
        float(distanceToLine)
    );
}

// Маска точки в узле сетки.
float CreateDotMask(
    double dx,
    double dy,
    float fx,
    float fy,
    float radius
)
{
    vec2 d = vec2(
        float(dx) / fx,
        float(dy) / fy
    );

    float dist = length(d);

    return 1.0 - smoothstep(radius, radius + 1.0, dist);
}

void main()
{
    // -------------------------------------------------------------------------
    // 1. Луч камеры в координатах относительно origin сетки
    // -------------------------------------------------------------------------

    // Эти точки уже восстановлены в vertex shader.
    //
    // Важно:
    // vertex shader сначала делает world - uGridOrigin в double,
    // и только потом передаёт локальные значения дальше.
    // Так мы уменьшаем влияние больших мировых координат.
    dvec3 rayOriginLocal = dvec3(vNearLocal);
    dvec3 rayFarLocal = dvec3(vFarLocal);

    dvec3 rayDir = normalize(rayFarLocal - rayOriginLocal);

    // В локальной системе относительно origin плоскость сетки проходит через 0.
    double denom = dot(rayDir, uGridNormal);

    // Гашение сетки при почти касательном взгляде.
    if (abs(denom) < uMinViewNormalDot)
    {
        discard;
    }

    // Пересечение луча с плоскостью:
    //
    // dot(rayOriginLocal + rayDir * t, normal) = 0
    //
    // t = -dot(rayOriginLocal, normal) / dot(rayDir, normal)
    double t = -dot(rayOriginLocal, uGridNormal) / denom;

    // Важно:
    // t < 0 означает, что точка пересечения с плоскостью сетки находится
    // перед near plane относительно текущего луча.
    //
    // Раньше мы сразу делали discard, но это ломает ручной depth clamp:
    // near-clamped область вообще не доходит до расчёта rawDepth
    // и не может быть окрашена в debug-режиме.
    //
    // Поэтому:
    // - если depth clamp выключен и debug выключен — можно discard'ить;
    // - если depth clamp включён или включён debug зон — продолжаем расчёт,
    //   потом rawDepth будет прижат к safeMinDepth.
    if (!uClampDepth && !uDebugDepthZones && t < 0.0)
    {
        discard;
    }

    dvec3 localPos = rayOriginLocal + rayDir * t;

    // -------------------------------------------------------------------------
    // 2. Координаты точки в локальной СК сетки
    // -------------------------------------------------------------------------

    double gx = dot(localPos, uGridAxisX);
    double gy = dot(localPos, uGridAxisY);

    // Ограничение bounded-сетки.
    if (uIsBounded)
    {
        if (
            gx < uGridBounds.x ||
            gy < uGridBounds.y ||
            gx > uGridBounds.z ||
            gy > uGridBounds.w
        )
        {
            discard;
        }
    }

    // -------------------------------------------------------------------------
    // 3. Расстояния до линий сетки
    // -------------------------------------------------------------------------

    double dxMinor = DistanceToGridLine(gx, uMinorStep);
    double dyMinor = DistanceToGridLine(gy, uMinorStep);

    double dxMajor = DistanceToGridLine(gx, uMajorStep);
    double dyMajor = DistanceToGridLine(gy, uMajorStep);

    float fgx = float(gx);
    float fgy = float(gy);

    // fwidth показывает изменение локальной координаты между соседними пикселями.
    // Через него делаем линии стабильными по экранной толщине.
    float fx = max(fwidth(fgx), 1e-6);
    float fy = max(fwidth(fgy), 1e-6);

    float minorMask = 0.0;
    float majorMask = 0.0;
    float boundaryMask = 0.0;
    float xAxisMask = 0.0;
    float yAxisMask = 0.0;

    // Оси рисуем как лучи из origin, а не как бесконечные прямые.
    const double dAxisDirectionEpsilon = 1e-9;

    float xAxisDirectionMask = gx >= -dAxisDirectionEpsilon ? 1.0 : 0.0;
    float yAxisDirectionMask = gy >= -dAxisDirectionEpsilon ? 1.0 : 0.0;

    if (uDrawDots)
    {
        // Режим точек.
        //
        // При этом оси и bounded-рамка остаются линиями,
        // чтобы направление и границы были читаемыми.
        if (uShowMinorGrid)
        {
            minorMask = CreateDotMask(
                dxMinor,
                dyMinor,
                fx,
                fy,
                uDotRadius
            );
        }

        if (uShowMajorGrid)
        {
            majorMask = CreateDotMask(
                dxMajor,
                dyMajor,
                fx,
                fy,
                uDotRadius * 1.35
            );
        }

        if (uShowAxes)
        {
            xAxisMask = xAxisDirectionMask * CreateLineMask(abs(gy), fy, uAxisThickness);
            yAxisMask = yAxisDirectionMask * CreateLineMask(abs(gx), fx, uAxisThickness);
        }
    }
    else
    {
        // Режим линий.
        if (uShowMinorGrid)
        {
            float minorX = CreateLineMask(dxMinor, fx, uMinorThickness);
            float minorY = CreateLineMask(dyMinor, fy, uMinorThickness);
            minorMask = max(minorX, minorY);
        }

        if (uShowMajorGrid)
        {
            float majorX = CreateLineMask(dxMajor, fx, uMajorThickness);
            float majorY = CreateLineMask(dyMajor, fy, uMajorThickness);
            majorMask = max(majorX, majorY);
        }

        if (uShowAxes)
        {
            xAxisMask = xAxisDirectionMask * CreateLineMask(abs(gy), fy, uAxisThickness);
            yAxisMask = yAxisDirectionMask * CreateLineMask(abs(gx), fx, uAxisThickness);
        }
    }

    // -------------------------------------------------------------------------
    // 4. Граничный прямоугольник bounded-сетки
    // -------------------------------------------------------------------------

    if (uIsBounded)
    {
        // Рисуем рамку ограниченной сетки.
        //
        // Цвет и толщину на этом шаге берём от больших линий.
        // Позже при необходимости можно вынести отдельные uniform-параметры:
        // borderColorTop/bottom и borderThickness.
        //
        // Так как выше уже был discard вне bounds,
        // эти четыре линии будут видны только по периметру прямоугольника.
        float boundaryMinX = CreateLineMask(abs(gx - uGridBounds.x), fx, uMajorThickness);
        float boundaryMaxX = CreateLineMask(abs(gx - uGridBounds.z), fx, uMajorThickness);
        float boundaryMinY = CreateLineMask(abs(gy - uGridBounds.y), fy, uMajorThickness);
        float boundaryMaxY = CreateLineMask(abs(gy - uGridBounds.w), fy, uMajorThickness);

        boundaryMask = max(
            max(boundaryMinX, boundaryMaxX),
            max(boundaryMinY, boundaryMaxY)
        );
    }

    // -------------------------------------------------------------------------
    // 5. Выбор цветов top/bottom
    // -------------------------------------------------------------------------

    bool isTopSide = denom < 0.0;

    vec4 planeColor = isTopSide ? uPlaneColorTop : uPlaneColorBottom;
    vec4 minorColor = isTopSide ? uMinorColorTop : uMinorColorBottom;
    vec4 majorColor = isTopSide ? uMajorColorTop : uMajorColorBottom;
    vec4 xAxisColor = isTopSide ? uXAxisColorTop : uXAxisColorBottom;
    vec4 yAxisColor = isTopSide ? uYAxisColorTop : uYAxisColorBottom;

    // Цвет рамки bounded-сетки пока берём от major-линий.
    vec4 boundaryColor = majorColor;

    float gridMask = max(
        max(minorMask, majorMask),
        max(max(boundaryMask, xAxisMask), yAxisMask)
    );

    if (!uDrawPlane && gridMask <= 0.001 && !uDebugDepthZones)
    {
        discard;
    }

    vec4 color = uDrawPlane
        ? planeColor
        : vec4(0.0, 0.0, 0.0, 0.0);

    // Приоритет:
    // plane -> minor -> major -> boundary -> axes.
    color = mix(color, minorColor, minorMask);
    color = mix(color, majorColor, majorMask);
    color = mix(color, boundaryColor, boundaryMask);
    color = mix(color, xAxisColor, xAxisMask);
    color = mix(color, yAxisColor, yAxisMask);

    if (color.a <= 0.001 && !uDebugDepthZones)
    {
        discard;
    }

    // -------------------------------------------------------------------------
    // 6. Глубина фрагмента сетки
    // -------------------------------------------------------------------------

    dvec3 worldPos = uGridOrigin + localPos;

    dvec4 clip = uViewProj * dvec4(worldPos, 1.0);
    double ndcZ = clip.z / clip.w;

    // OpenGL NDC z: [-1, 1]
    // Depth buffer: [0, 1]
    double rawDepth = ndcZ * 0.5 + 0.5;

    // Безопасный внутренний диапазон depth buffer.
    //
    // Не используем ровно 0.0 и 1.0, потому что на некоторых драйверах
    // фрагменты на самой границе могут быть отрезаны.
    double safeMinDepth = uSafeDepthEpsilon;
    double safeMaxDepth = 1.0 - uSafeDepthEpsilon;

    // Страховка от некорректного epsilon.
    safeMinDepth = clamp(safeMinDepth, 0.0, 0.499999);
    safeMaxDepth = clamp(safeMaxDepth, 0.500001, 1.0);

    // Определяем зону глубины до clamp.
    //
    // Это удобно для отладки:
    // - near zone: точка пересечения ушла перед near plane;
    // - normal zone: точка в нормальном диапазоне сцены;
    // - far zone: точка ушла за far plane.
    bool bNearDepthZone = rawDepth < safeMinDepth;
    bool bFarDepthZone = rawDepth > safeMaxDepth;
    bool bNormalDepthZone = !bNearDepthZone && !bFarDepthZone;

    double depth = rawDepth;

    if (uClampDepth)
    {
        // Ручной depth clamp.
        //
        // Фрагмент не отбрасываем, а прижимаем его глубину
        // к безопасному внутреннему диапазону.
        depth = clamp(rawDepth, safeMinDepth, safeMaxDepth);
    }
    else
    {
        // Старое поведение для сравнения:
        // если depth вне диапазона — фрагмент отбрасывается.
        if (rawDepth < safeMinDepth || rawDepth > safeMaxDepth)
        {
            discard;
        }
    }

    // Отладочный режим раскраски зон.
    //
    // В этом режиме временно игнорируем линии/точки сетки.
    // Это помогает увидеть, какая часть плоскости сетки:
    // - прижалась к near;
    // - находится в нормальном диапазоне;
    // - прижалась к far.
    if (uDebugDepthZones)
    {
        if (bNearDepthZone)
        {
            // Красный: фрагменты перед near plane.
            outColor = vec4(1.0, 0.1, 0.1, 0.85);
        }
        else if (bFarDepthZone)
        {
            // Синий: фрагменты за far plane.
            outColor = vec4(0.1, 0.25, 1.0, 0.85);
        }
        else
        {
            // Зелёный: нормальный диапазон глубины.
            outColor = vec4(0.1, 0.9, 0.25, 0.65);
        }

        gl_FragDepth = float(depth);
        return;
    }

    // Важно:
    // сетка вычисляет свою глубину аналитически.
    //
    // А запись в depth buffer должна быть отключена со стороны OpenGL
    // через glDepthMask(GL_FALSE), чтобы сетка участвовала в Z-test,
    // но не портила depth buffer сцены.
    gl_FragDepth = float(depth);

    outColor = color;
}