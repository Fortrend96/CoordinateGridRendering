#version 430 core

layout(location = 0) out vec4 outColor;

uniform dmat4 uViewProj;
uniform dmat4 uInvViewProj;

uniform dvec2 uViewportSize;

uniform dvec3 uGridOrigin;
uniform dvec3 uGridAxisX;
uniform dvec3 uGridAxisY;
uniform dvec3 uGridNormal;

uniform double uMinViewNormalDot;

uniform bool uIsBounded;
uniform dvec4 uGridBounds; // minX, minY, maxX, maxY

uniform bool uDrawDots;
uniform float uDotRadius;

uniform double uMinorStep;
uniform double uMajorStep;

uniform float uMinorThickness;
uniform float uMajorThickness;
uniform float uAxisThickness;

uniform vec4 uPlaneColor;
uniform vec4 uMinorColor;
uniform vec4 uMajorColor;
uniform vec4 uXAxisColor;
uniform vec4 uYAxisColor;

double DistanceToGridLine(double x, double step)
{
    double nearestLine = round(x / step) * step;
    return abs(x - nearestLine);
}

float CreateLineMask(double distanceToLine, float pixelWidth, float thickness)
{
    return 1.0 - smoothstep(
        0.0,
        pixelWidth * thickness,
        float(distanceToLine)
    );
}

float CreateDotMask(
    double dx,
    double dy,
    float fx,
    float fy,
    float radius
)
{
    // Переводим расстояние до узла в условные экранные единицы.
    //
    // Если dx примерно равен fx, значит по X точка удалена примерно на 1 пиксель.
    // То же самое для dy/fy.
    vec2 d = vec2(
        float(dx) / fx,
        float(dy) / fy
    );

    float dist = length(d);

    // radius — радиус точки в условных пикселях.
    // +1.0 даёт мягкий антиалиасинг края.
    return 1.0 - smoothstep(radius, radius + 1.0, dist);
}

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;

    vec2 ndc = vec2(
        fragCoord.x / float(uViewportSize.x) * 2.0 - 1.0,
        fragCoord.y / float(uViewportSize.y) * 2.0 - 1.0
    );

    dvec4 nearClip = dvec4(ndc, -1.0, 1.0);
    dvec4 farClip  = dvec4(ndc,  1.0, 1.0);

    dvec4 nearWorld = uInvViewProj * nearClip;
    dvec4 farWorld  = uInvViewProj * farClip;

    nearWorld /= nearWorld.w;
    farWorld  /= farWorld.w;

    dvec3 rayOrigin = nearWorld.xyz;
    dvec3 rayDir = normalize(farWorld.xyz - nearWorld.xyz);

    double denom = dot(rayDir, uGridNormal);

    // Не рисуем сетку, если смотрим почти вдоль её плоскости.
    if (abs(denom) < uMinViewNormalDot)
    {
        discard;
    }

    double t = dot(uGridOrigin - rayOrigin, uGridNormal) / denom;

    if (t < 0.0)
    {
        discard;
    }

    dvec3 worldPos = rayOrigin + rayDir * t;

    dvec3 local = worldPos - uGridOrigin;

    double gx = dot(local, uGridAxisX);
    double gy = dot(local, uGridAxisY);

    // Ограниченная сетка.
    //
    // Границы заданы в локальных координатах сетки:
    // uGridBounds.x = minX
    // uGridBounds.y = minY
    // uGridBounds.z = maxX
    // uGridBounds.w = maxY
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

    if (uDrawDots)
    {
        // Режим точек.
        //
        // Узел сетки — это место, где одновременно:
        // gx близок к кратному step
        // gy близок к кратному step
        minorMask = CreateDotMask(
            dxMinor,
            dyMinor,
            fx,
            fy,
            uDotRadius
        );

        majorMask = CreateDotMask(
            dxMajor,
            dyMajor,
            fx,
            fy,
            uDotRadius * 1.35
        );

        // Оси в режиме точек пока оставляем линиями:
        // так лучше видно направление X/Y.
        xAxisMask = CreateLineMask(abs(gy), fy, uAxisThickness);
        yAxisMask = CreateLineMask(abs(gx), fx, uAxisThickness);
    }
    else
    {
        // Режим линий.
        float minorX = CreateLineMask(dxMinor, fx, uMinorThickness);
        float minorY = CreateLineMask(dyMinor, fy, uMinorThickness);
        minorMask = max(minorX, minorY);

        float majorX = CreateLineMask(dxMajor, fx, uMajorThickness);
        float majorY = CreateLineMask(dyMajor, fy, uMajorThickness);
        majorMask = max(majorX, majorY);

        xAxisMask = CreateLineMask(abs(gy), fy, uAxisThickness);
        yAxisMask = CreateLineMask(abs(gx), fx, uAxisThickness);
    }

    vec4 color = uPlaneColor;

    color = mix(color, uMinorColor, minorMask);
    color = mix(color, uMajorColor, majorMask);
    color = mix(color, uXAxisColor, xAxisMask);
    color = mix(color, uYAxisColor, yAxisMask);

    if (color.a <= 0.001)
    {
        discard;
    }

    dvec4 clip = uViewProj * dvec4(worldPos, 1.0);
    double ndcZ = clip.z / clip.w;

    double depth = ndcZ * 0.5 + 0.5;

    if (depth < 0.0 || depth > 1.0)
    {
        discard;
    }

    gl_FragDepth = float(depth);
    outColor = color;
}