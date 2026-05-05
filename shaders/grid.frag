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

// Цвета верхней стороны сетки.
uniform vec4 uPlaneColorTop;
uniform vec4 uMinorColorTop;
uniform vec4 uMajorColorTop;
uniform vec4 uXAxisColorTop;
uniform vec4 uYAxisColorTop;

// Цвета нижней стороны сетки.
uniform vec4 uPlaneColorBottom;
uniform vec4 uMinorColorBottom;
uniform vec4 uMajorColorBottom;
uniform vec4 uXAxisColorBottom;
uniform vec4 uYAxisColorBottom;

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

    // Координата пикселя в OpenGL NDC.
    vec2 ndc = vec2(
        fragCoord.x / float(uViewportSize.x) * 2.0 - 1.0,
        fragCoord.y / float(uViewportSize.y) * 2.0 - 1.0
    );

    // OpenGL: near plane = -1, far plane = +1.
    dvec4 nearClip = dvec4(ndc, -1.0, 1.0);
    dvec4 farClip  = dvec4(ndc,  1.0, 1.0);

    dvec4 nearWorld = uInvViewProj * nearClip;
    dvec4 farWorld  = uInvViewProj * farClip;

    nearWorld /= nearWorld.w;
    farWorld  /= farWorld.w;

    dvec3 rayOrigin = nearWorld.xyz;
    dvec3 rayDir = normalize(farWorld.xyz - nearWorld.xyz);

    double denom = dot(rayDir, uGridNormal);

    // Если смотрим почти вдоль плоскости, сетку не рисуем:
    // в этом положении она визуально становится нестабильной.
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

    // Ограничение прямоугольником в локальной СК сетки.
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
        // Режим точек в узлах сетки.
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

        // Оси оставляем линиями, чтобы направление X/Y было хорошо видно.
        xAxisMask = CreateLineMask(abs(gy), fx, uAxisThickness);
        yAxisMask = CreateLineMask(abs(gx), fy, uAxisThickness);
    }
    else
    {
        float minorX = CreateLineMask(dxMinor, fx, uMinorThickness);
        float minorY = CreateLineMask(dyMinor, fy, uMinorThickness);
        minorMask = max(minorX, minorY);

        float majorX = CreateLineMask(dxMajor, fx, uMajorThickness);
        float majorY = CreateLineMask(dyMajor, fy, uMajorThickness);
        majorMask = max(majorX, majorY);

        xAxisMask = CreateLineMask(abs(gy), fy, uAxisThickness);
        yAxisMask = CreateLineMask(abs(gx), fx, uAxisThickness);
    }

    // denom < 0 означает, что луч смотрит на сторону нормали.
    bool isTopSide = denom < 0.0;

    vec4 planeColor = isTopSide ? uPlaneColorTop : uPlaneColorBottom;
    vec4 minorColor = isTopSide ? uMinorColorTop : uMinorColorBottom;
    vec4 majorColor = isTopSide ? uMajorColorTop : uMajorColorBottom;
    vec4 xAxisColor = isTopSide ? uXAxisColorTop : uXAxisColorBottom;
    vec4 yAxisColor = isTopSide ? uYAxisColorTop : uYAxisColorBottom;

    vec4 color = planeColor;

    color = mix(color, minorColor, minorMask);
    color = mix(color, majorColor, majorMask);
    color = mix(color, xAxisColor, xAxisMask);
    color = mix(color, yAxisColor, yAxisMask);

    if (color.a <= 0.001)
    {
        discard;
    }

    dvec4 clip = uViewProj * dvec4(worldPos, 1.0);
    double ndcZ = clip.z / clip.w;

    // OpenGL NDC z: [-1, 1]
    // Depth buffer: [0, 1]
    double depth = ndcZ * 0.5 + 0.5;

    // По заданию: если глубина выходит за область отсечения,
    // не обязательно отбрасывать фрагмент — можно прижать её к границе.
    depth = clamp(depth, 0.0, 1.0);

    gl_FragDepth = float(depth);
    outColor = color;
}