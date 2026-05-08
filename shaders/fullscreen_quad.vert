#version 430 core

void main()
{
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