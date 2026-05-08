#version 430 core

layout(location = 0) out vec4 outColor;

in vec3 vWorldNormal;

// Базовый цвет объекта.
uniform vec3 uBaseColor;

// Направление света в world-space.
uniform vec3 uLightDirectionWorld;

void main()
{
    vec3 vNormal = normalize(vWorldNormal);
    vec3 vLightDir = normalize(-uLightDirectionWorld);

    float fDiffuse = max(dot(vNormal, vLightDir), 0.0);

    // Простое ламбертово освещение.
    // Нужна только базовая читаемость формы объектов.
    float fLighting = 0.30 + 0.70 * fDiffuse;

    outColor = vec4(uBaseColor * fLighting, 1.0);
}
