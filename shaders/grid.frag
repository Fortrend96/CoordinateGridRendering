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

uniform bool uClampDepth;

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
    //
    // Если dx примерно равен fx, значит по X точка удалена примерно на 1 пиксель.
    // То же самое для dy / fy.
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

    // OpenGL NDC:
    // x = -1 слева, +1 справа
    // y = -1 снизу, +1 сверху
    vec2 ndc = vec2(
        fragCoord.x / float(uViewportSize.x) * 2.0 - 1.0,
        fragCoord.y / float(uViewportSize.y) * 2.0 - 1.0
    );

    // OpenGL convention:
    // near plane = -1
    // far plane  =  1
    dvec4 nearClip = dvec4(ndc, -1.0, 1.0);
    dvec4 farClip  = dvec4(ndc,  1.0, 1.0);

    dvec4 nearWorld = uInvViewProj * nearClip;
    dvec4 farWorld  = uInvViewProj * farClip;

    nearWorld /= nearWorld.w;
    farWorld  /= farWorld.w;

    dvec3 rayOrigin = nearWorld.xyz;
    dvec3 rayDir = normalize(farWorld.xyz - nearWorld.xyz);

    // Пересечение луча с плоскостью сетки.
    double denom = dot(rayDir, uGridNormal);

    // Если смотрим почти вдоль плоскости, сетку не рисуем.
    // Это защищает от визуального шума при взгляде "с ребра".
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

    // Переход в локальную систему координат сетки.
    //
    // Важно, что worldPos и uGridOrigin — double.
    // Это уменьшает потери точности при больших мировых координатах.
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

    // fwidth показывает изменение величины между соседними пикселями.
    // Через него делаем линии более стабильными на экране.
    float fx = max(fwidth(fgx), 1e-6);
    float fy = max(fwidth(fgy), 1e-6);

    // -------------------------------------------------------------------------
    // LOD / fade плотной сетки
    // -------------------------------------------------------------------------
    //
    // При сильном отдалении малый шаг сетки может стать меньше пикселя.
    // Тогда линии начинают сливаться и появляется муар / "серая каша".
    //
    // fx/fy показывают, сколько grid units приходится примерно на один пиксель.
    // Значит:
    //
    //     step / fwidth
    //
    // даёт примерный размер шага в пикселях.
    //
    // Если шаг занимает слишком мало пикселей, плавно гасим соответствующий слой.
    float minorStepPixelsX = float(uMinorStep) / fx;
    float minorStepPixelsY = float(uMinorStep) / fy;

    float majorStepPixelsX = float(uMajorStep) / fx;
    float majorStepPixelsY = float(uMajorStep) / fy;

    float minorStepPixels = min(minorStepPixelsX, minorStepPixelsY);
    float majorStepPixels = min(majorStepPixelsX, majorStepPixelsY);

    // Меньше 3 пикселей между линиями — почти не рисуем.
    // Больше 8 пикселей — рисуем полностью.
    float minorLodFade = smoothstep(3.0, 8.0, minorStepPixels);
    float majorLodFade = smoothstep(3.0, 8.0, majorStepPixels);

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

        // Оси оставляем линиями даже в режиме точек,
        // чтобы направление X/Y было хорошо видно.
        //
        // Важно:
        // - для abs(gy) используем fy;
        // - для abs(gx) используем fx.
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

    // Гасим слишком плотные слои сетки вдали.
    //
    // Оси не гасим: они должны оставаться видны даже при сильном отдалении.
    minorMask *= minorLodFade;
    majorMask *= majorLodFade;

    // denom < 0 означает, что луч смотрит на сторону нормали.
    bool isTopSide = denom < 0.0;

    vec4 planeColor = isTopSide ? uPlaneColorTop : uPlaneColorBottom;
    vec4 minorColor = isTopSide ? uMinorColorTop : uMinorColorBottom;
    vec4 majorColor = isTopSide ? uMajorColorTop : uMajorColorBottom;
    vec4 xAxisColor = isTopSide ? uXAxisColorTop : uXAxisColorBottom;
    vec4 yAxisColor = isTopSide ? uYAxisColorTop : uYAxisColorBottom;

    // Приоритет:
    // плоскость -> малые линии/точки -> большие линии/точки -> ось X -> ось Y.
    vec4 color = planeColor;

    color = mix(color, minorColor, minorMask);
    color = mix(color, majorColor, majorMask);
    color = mix(color, xAxisColor, xAxisMask);
    color = mix(color, yAxisColor, yAxisMask);

    if (color.a <= 0.001)
    {
        discard;
    }

    // Настоящая глубина точки сетки.
    dvec4 clip = uViewProj * dvec4(worldPos, 1.0);
    double ndcZ = clip.z / clip.w;

    // OpenGL NDC z: [-1, 1]
    // Depth buffer: [0, 1]
    double depth = ndcZ * 0.5 + 0.5;

    // Важный момент:
    // безусловный clamp может создавать визуальные артефакты,
    // потому что фрагменты за near/far всё равно начинают рисоваться.
    //
    // Поэтому clamp сделан отдельным режимом.
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