#version 430 core

// Fullscreen triangle.
//
// Мы не создаём vertex buffer.
// Вершины генерируются через gl_VertexID.
const vec2 kPositions[3] = vec2[3](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

// Обратная view-projection матрица.
// Используется здесь, в vertex shader, а не во fragment shader,
// чтобы не делать dmat4 * dvec4 для каждого пикселя.
uniform dmat4 uInvViewProj;

// Origin сетки.
//
// Мы сразу переводим near/far points в локальные координаты относительно origin.
// Это уменьшает потерю точности при больших мировых координатах.
uniform dvec3 uGridOrigin;

// Локальная точка луча на near plane.
// Передаётся во fragment shader.
noperspective out vec3 vNearLocal;

// Локальная точка луча на far plane.
// Передаётся во fragment shader.
noperspective out vec3 vFarLocal;

void main()
{
    vec2 position = kPositions[gl_VertexID];

    // Fullscreen triangle в clip-space.
    gl_Position = vec4(position, 0.0, 1.0);

    // Для fullscreen triangle position.xy фактически является NDC.
    //
    // OpenGL:
    // near z = -1
    // far  z =  1
    dvec4 nearClip = dvec4(position, -1.0, 1.0);
    dvec4 farClip  = dvec4(position,  1.0, 1.0);

    dvec4 nearWorld = uInvViewProj * nearClip;
    dvec4 farWorld  = uInvViewProj * farClip;

    nearWorld /= nearWorld.w;
    farWorld  /= farWorld.w;

    // Важно:
    // в fragment shader передаём не абсолютные world coordinates,
    // а координаты относительно uGridOrigin.
    //
    // Это помогает при случаях вроде:
    // origin = (1_000_000, -2_000_000, 500_000)
    dvec3 nearLocal = nearWorld.xyz - uGridOrigin;
    dvec3 farLocal  = farWorld.xyz  - uGridOrigin;

    // Передаём как float-векторы.
    // Основная высокая точность уже сохранена на операции world - origin,
    // которая выполнена в double.
    vNearLocal = vec3(nearLocal);
    vFarLocal  = vec3(farLocal);
}