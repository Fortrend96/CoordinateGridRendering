#version 430 core

// Fullscreen triangle.
//
// Мы не создаём VBO с вершинами.
// Вместо этого используем gl_VertexID: 0, 1, 2.
//
// Эти три вершины образуют один большой треугольник,
// который покрывает весь экран.
const vec2 kPositions[3] = vec2[3](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

void main()
{
    vec2 position = kPositions[gl_VertexID];

    // Координаты сразу задаются в clip-space.
    // Матрицы view/projection здесь не нужны.
    gl_Position = vec4(position, 0.0, 1.0);
}