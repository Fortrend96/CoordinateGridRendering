#version 430 core

layout(location = 0) in dvec3 aLocalPosition;
layout(location = 1) in vec4 aColor;

uniform dmat4 uViewProj;
uniform dvec3 uGridOrigin;

out vec4 vColor;

void main()
{
    // aLocalPosition хранится как смещение относительно origin сетки.
    dvec3 worldPosition = uGridOrigin + aLocalPosition;

    dvec4 clipPosition = uViewProj * dvec4(worldPosition, 1.0);

    gl_Position = vec4(clipPosition);
    vColor = aColor;
}
