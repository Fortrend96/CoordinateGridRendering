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
// Матрицы и параметры viewport
// -----------------------------------------------------------------------------

// View-projection матрица.
// Нужна для вычисления настоящей глубины точки сетки.
uniform dmat4 uViewProj;

// -----------------------------------------------------------------------------
// Геометрия сетки
// -----------------------------------------------------------------------------

// Начало сетки в world-space.
uniform dvec3 uGridOrigin;

// Ось X сетки в world-space.
// Предполагаем, что нормализована.
uniform dvec3 uGridAxisX;

// Ось Y сетки в world-space.
// Предполагаем, что нормализована.
uniform dvec3 uGridAxisY;

// Нормаль плоскости сетки в world-space.
// Предполагаем, что нормализована.
uniform dvec3 uGridNormal;

// Если abs(dot(rayDir, normal)) меньше этого значения,
// сетку не рисуем: взгляд почти вдоль плоскости.
uniform double uMinViewNormalDot;

// -----------------------------------------------------------------------------
// Режимы отображения
// -----------------------------------------------------------------------------

// false — depth за near/far отбрасываем;
// true  — depth прижимаем к [0; 1].
uniform bool uClampDepth;

// false — рисуем только линии/оси;
// true  — дополнительно заливаем плоскость сетки.
uniform bool uDrawPlane;

// Ограниченная / бесконечная сетка.
uniform bool uIsBounded;

// Границы ограниченной сетки в локальной СК:
// x = minX
// y = minY
// z = maxX
// w = maxY
uniform dvec4 uGridBounds;

// false — рисуем линии;
// true  — рисуем точки в узлах.
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
// Теперь он может быть адаптивным и меняться при zoom.
uniform double uMinorStep;

// Большой шаг сетки.
// Обычно равен uMinorStep * 10.
uniform double uMajorStep;

// Толщина малых линий.
uniform float uMinorThickness;

// Толщина больших линий.
uniform float uMajorThickness;

// Толщина осей.
uniform float uAxisThickness;

// Радиус точек в режиме dots.
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
    // 1. Луч камеры в локальных координатах сетки
    // -------------------------------------------------------------------------

    // Эти точки уже были восстановлены в vertex shader.
    // Здесь мы больше не делаем uInvViewProj * clip per-pixel.
    dvec3 rayOriginLocal = dvec3(vNearLocal);
    dvec3 rayFarLocal = dvec3(vFarLocal);

    dvec3 rayDir = normalize(rayFarLocal - rayOriginLocal);

    // Плоскость сетки в локальных координатах проходит через (0, 0, 0),
    // потому что мы работаем относительно uGridOrigin.
    double denom = dot(rayDir, uGridNormal);

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

    if (t < 0.0)
    {
        discard;
    }

    dvec3 localPos = rayOriginLocal + rayDir * t;

    // -------------------------------------------------------------------------
    // 2. Локальные координаты сетки
    // -------------------------------------------------------------------------

    double gx = dot(localPos, uGridAxisX);
    double gy = dot(localPos, uGridAxisY);

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
    // 3. Расстояния до линий
    // -------------------------------------------------------------------------

    double dxMinor = DistanceToGridLine(gx, uMinorStep);
    double dyMinor = DistanceToGridLine(gy, uMinorStep);

    double dxMajor = DistanceToGridLine(gx, uMajorStep);
    double dyMajor = DistanceToGridLine(gy, uMajorStep);

    float fgx = float(gx);
    float fgy = float(gy);

    float fx = max(fwidth(fgx), 1e-6);
    float fy = max(fwidth(fgy), 1e-6);

    float minorMask = 0.0;
    float majorMask = 0.0;
    float xAxisMask = 0.0;
    float yAxisMask = 0.0;

    // Оси рисуем как лучи из origin, а не как бесконечные прямые.
    const double dAxisDirectionEpsilon = 1e-9;

    float xAxisDirectionMask = gx >= -dAxisDirectionEpsilon ? 1.0 : 0.0;
    float yAxisDirectionMask = gy >= -dAxisDirectionEpsilon ? 1.0 : 0.0;

    if (uDrawDots)
    {
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
    // 4. Цвета
    // -------------------------------------------------------------------------

    bool isTopSide = denom < 0.0;

    vec4 planeColor = isTopSide ? uPlaneColorTop : uPlaneColorBottom;
    vec4 minorColor = isTopSide ? uMinorColorTop : uMinorColorBottom;
    vec4 majorColor = isTopSide ? uMajorColorTop : uMajorColorBottom;
    vec4 xAxisColor = isTopSide ? uXAxisColorTop : uXAxisColorBottom;
    vec4 yAxisColor = isTopSide ? uYAxisColorTop : uYAxisColorBottom;

    float gridMask = max(max(minorMask, majorMask), max(xAxisMask, yAxisMask));

    if (!uDrawPlane && gridMask <= 0.001)
    {
        discard;
    }

    vec4 color = uDrawPlane
        ? planeColor
        : vec4(0.0, 0.0, 0.0, 0.0);

    color = mix(color, minorColor, minorMask);
    color = mix(color, majorColor, majorMask);
    color = mix(color, xAxisColor, xAxisMask);
    color = mix(color, yAxisColor, yAxisMask);

    if (color.a <= 0.001)
    {
        discard;
    }

    // -------------------------------------------------------------------------
    // 5. Глубина
    // -------------------------------------------------------------------------

    dvec3 worldPos = uGridOrigin + localPos;

    dvec4 clip = uViewProj * dvec4(worldPos, 1.0);
    double ndcZ = clip.z / clip.w;

    double depth = ndcZ * 0.5 + 0.5;

    if (uClampDepth)
    {
        depth = clamp(depth, 0.0, 1.0);
    }
    else
    {
        if (depth < 0.0 || depth > 1.0)
        {
            discard;
        }
    }

    gl_FragDepth = float(depth);
    outColor = color;
}