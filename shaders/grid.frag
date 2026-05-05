#version 430 core

layout(location = 0) out vec4 outColor;

// Матрица view-projection переводит world-space координаты в clip-space.
uniform dmat4 uViewProj;

// Обратная view-projection матрица нужна, чтобы по пикселю экрана
// восстановить мировой луч камеры.
uniform dmat4 uInvViewProj;

// Размер framebuffer в пикселях.
uniform dvec2 uViewportSize;

// Параметры плоскости сетки.
uniform dvec3 uGridOrigin;
uniform dvec3 uGridAxisX;
uniform dvec3 uGridAxisY;
uniform dvec3 uGridNormal;

// Если abs(dot(rayDir, normal)) меньше этого порога,
// значит смотрим на сетку почти с ребра.
// В таком случае сетку лучше не рисовать.
uniform double uMinViewNormalDot;

// Шаги сетки.
uniform double uMinorStep;
uniform double uMajorStep;

// Толщины линий.
// Это не строго пиксели, но за счёт fwidth ведут себя как экранно-зависимая толщина.
uniform float uMinorThickness;
uniform float uMajorThickness;
uniform float uAxisThickness;

// Цвета.
uniform vec4 uPlaneColor;
uniform vec4 uMinorColor;
uniform vec4 uMajorColor;
uniform vec4 uXAxisColor;
uniform vec4 uYAxisColor;

// Возвращает расстояние от координаты x до ближайшей линии сетки,
// кратной step.
//
// Например, если step = 1.0, линии находятся в:
// ..., -2, -1, 0, 1, 2, ...
double DistanceToGridLine(double x, double step)
{
    double nearestLine = round(x / step) * step;
    return abs(x - nearestLine);
}

void main()
{
    // 1. Координата текущего пикселя.
    vec2 fragCoord = gl_FragCoord.xy;

    // 2. Переводим пиксель в NDC.
    //
    // В OpenGL:
    // x = -1 слева, +1 справа
    // y = -1 снизу, +1 сверху
    vec2 ndc = vec2(
        fragCoord.x / float(uViewportSize.x) * 2.0 - 1.0,
        fragCoord.y / float(uViewportSize.y) * 2.0 - 1.0
    );

    // 3. Берём две точки на near/far plane.
    //
    // Для OpenGL NDC z:
    // near = -1
    // far  =  1
    dvec4 nearClip = dvec4(ndc, -1.0, 1.0);
    dvec4 farClip  = dvec4(ndc,  1.0, 1.0);

    // 4. Переводим эти точки обратно в world-space.
    dvec4 nearWorld = uInvViewProj * nearClip;
    dvec4 farWorld  = uInvViewProj * farClip;

    // Perspective divide.
    nearWorld /= nearWorld.w;
    farWorld  /= farWorld.w;

    // 5. Строим луч камеры для текущего пикселя.
    dvec3 rayOrigin = nearWorld.xyz;
    dvec3 rayDir = normalize(farWorld.xyz - nearWorld.xyz);

    // 6. Пересекаем луч с плоскостью сетки.
    //
    // denom показывает, насколько луч направлен к нормали плоскости.
    // Если denom близок к 0, луч почти параллелен плоскости.
    double denom = dot(rayDir, uGridNormal);

    // Отсекаем случай, когда сетка видна почти "с ребра".
    // Это защищает от визуально плохих случаев: мерцания, огромных t,
    // сильного сжатия линий вдали.
    if (abs(denom) < uMinViewNormalDot)
    {
        discard;
    }

    double t = dot(uGridOrigin - rayOrigin, uGridNormal) / denom;

    // Если t < 0, плоскость находится позади луча.
    if (t < 0.0)
    {
        discard;
    }

    dvec3 worldPos = rayOrigin + rayDir * t;

    // 7. Переводим точку из world-space в локальные координаты сетки.
    //
    // Здесь важно использовать double: при больших мировых координатах
    // float может терять точность на операции worldPos - uGridOrigin.
    dvec3 local = worldPos - uGridOrigin;

    // В этой версии считаем, что оси сетки:
    // - нормализованы;
    // - взаимно перпендикулярны.
    double gx = dot(local, uGridAxisX);
    double gy = dot(local, uGridAxisY);

    // 8. Расстояния до малых и больших линий.
    double dxMinor = DistanceToGridLine(gx, uMinorStep);
    double dyMinor = DistanceToGridLine(gy, uMinorStep);

    double dxMajor = DistanceToGridLine(gx, uMajorStep);
    double dyMajor = DistanceToGridLine(gy, uMajorStep);

    // 9. fwidth работает с float.
    // Оно оценивает изменение величины между соседними пикселями.
    //
    // Благодаря этому толщина линий становится экранно-стабильнее.
    float fgx = float(gx);
    float fgy = float(gy);

    float fx = max(fwidth(fgx), 1e-6);
    float fy = max(fwidth(fgy), 1e-6);

    // 10. Маска малых линий.
    //
    // minorX — вертикальные линии, где gx кратен uMinorStep.
    // minorY — горизонтальные линии, где gy кратен uMinorStep.
    float minorX = 1.0 - smoothstep(0.0, fx * uMinorThickness, float(dxMinor));
    float minorY = 1.0 - smoothstep(0.0, fy * uMinorThickness, float(dyMinor));
    float minorMask = max(minorX, minorY);

    // 11. Маска больших линий.
    float majorX = 1.0 - smoothstep(0.0, fx * uMajorThickness, float(dxMajor));
    float majorY = 1.0 - smoothstep(0.0, fy * uMajorThickness, float(dyMajor));
    float majorMask = max(majorX, majorY);

    // 12. Оси.
    //
    // Ось X находится там, где gy = 0.
    // Ось Y находится там, где gx = 0.
    float xAxisMask = 1.0 - smoothstep(0.0, fy * uAxisThickness, abs(fgy));
    float yAxisMask = 1.0 - smoothstep(0.0, fx * uAxisThickness, abs(fgx));

    // 13. Смешиваем цвета по приоритету:
    //
    // фон плоскости
    // -> малые линии
    // -> большие линии
    // -> ось X
    // -> ось Y
    vec4 color = uPlaneColor;

    color = mix(color, uMinorColor, minorMask);
    color = mix(color, uMajorColor, majorMask);
    color = mix(color, uXAxisColor, xAxisMask);
    color = mix(color, uYAxisColor, yAxisMask);

    if (color.a <= 0.001)
    {
        discard;
    }

    // 14. Считаем настоящую глубину точки сетки.
    //
    // Хотя геометрически мы рисуем fullscreen triangle,
    // depth должен соответствовать точке worldPos на плоскости сетки.
    dvec4 clip = uViewProj * dvec4(worldPos, 1.0);
    double ndcZ = clip.z / clip.w;

    // В OpenGL NDC z находится в диапазоне [-1; 1].
    // Depth buffer использует [0; 1].
    double depth = ndcZ * 0.5 + 0.5;

    if (depth < 0.0 || depth > 1.0)
    {
        discard;
    }

    gl_FragDepth = float(depth);
    outColor = color;
}