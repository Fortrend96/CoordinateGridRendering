#version 430 core

// Локальные точки луча относительно uGridOrigin.
// Эти значения интерполируются по fullscreen quad и используются fragment shader'ом.
noperspective out vec3 vNearLocal;
noperspective out vec3 vFarLocal;

// Обратная view-projection матрица
// Нужна для восстановления world-space точки из clip-space.
uniform dmat4 uInvViewProj;

// Начало сетки в world-space.
// Вычитаем его уже в vertex shader, чтобы fragment shader работал
// с локальными координатами относительно origin сетки.
uniform dvec3 uGridOrigin;

void main()
{
    // Fullscreen quad, состоящий из двух треугольников.
    //
    // Координаты заданы сразу в clip-space:
    //
    // (-1,  1) -------- ( 1,  1)
    //     |              |
    //     |              |
    // (-1, -1) -------- ( 1, -1)
    //
    // Треугольники:
    // 0, 1, 2
    // 3, 4, 5
    const vec2 arrPositions[6] = vec2[6](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2( 1.0,  1.0),

        vec2(-1.0, -1.0),
        vec2( 1.0,  1.0),
        vec2(-1.0,  1.0)
    );

    vec2 vPosition = arrPositions[gl_VertexID];

    // Отдаём позицию вершины fullscreen quad прямо в clip-space.
    gl_Position = vec4(vPosition, 0.0, 1.0);

    // Для текущей вершины fullscreen quad восстанавливаем две точки:
    // - точку на near plane;
    // - точку на far plane.
    //
    // Потом fragment shader получит интерполированные значения
    // и построит луч через конкретный пиксель.
    dvec4 vNearClip = dvec4(
        double(vPosition.x),
        double(vPosition.y),
        -1.0,
        1.0
    );

    dvec4 vFarClip = dvec4(
        double(vPosition.x),
        double(vPosition.y),
        1.0,
        1.0
    );

    dvec4 vNearWorld = uInvViewProj * vNearClip;
    dvec4 vFarWorld = uInvViewProj * vFarClip;

    vNearWorld /= vNearWorld.w;
    vFarWorld /= vFarWorld.w;

    // Переводим координаты в систему относительно origin сетки.
    //
    // Это уменьшает проблемы точности при больших world-space координатах.
    dvec3 vNearLocalDouble = vNearWorld.xyz - uGridOrigin;
    dvec3 vFarLocalDouble = vFarWorld.xyz - uGridOrigin;
    
    // Передаём дальше как vec3, потому что interpolation stage работает
    // с float-типами. Основная компенсация больших координат уже сделана выше:
    // world - uGridOrigin.
    vNearLocal = vec3(vNearLocalDouble);
    vFarLocal = vec3(vFarLocalDouble);
}

