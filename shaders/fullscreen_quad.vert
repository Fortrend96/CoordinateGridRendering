#version 430 core

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

    gl_Position = vec4(arrPositions[gl_VertexID], 0.0, 1.0);
}