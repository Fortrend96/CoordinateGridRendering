#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

// Матрица объекта.
uniform mat4 uModel;

// Общая матрица вида и проекции.
uniform mat4 uViewProj;

out vec3 vWorldNormal;

void main()
{
    vec4 vWorldPosition = uModel * vec4(aPosition, 1.0);

    // Для тестовой сцены мы используем только повороты, переносы и равномерные масштабы,
    // поэтому достаточно взять mat3(uModel).
    vWorldNormal = normalize(mat3(uModel) * aNormal);

    gl_Position = uViewProj * vWorldPosition;
}
