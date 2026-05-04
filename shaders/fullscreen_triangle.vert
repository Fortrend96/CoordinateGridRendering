#version 430 core

// Мы не используем VBO.
// Вместо этого берём номер вершины через gl_VertexID.
//
// Fullscreen triangle состоит из трёх вершин.
// Эти координаты специально выходят за пределы [-1; 1],
// чтобы один треугольник гарантированно покрыл весь экран.
const vec2 kPositions[3] = vec2[3](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

void main()
{
    vec2 position = kPositions[gl_VertexID];

    // gl_Position задаётся сразу в clip-space.
    // Для fullscreen triangle не нужны view/projection матрицы.
    gl_Position = vec4(position, 0.0, 1.0);
}
