#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

// Матрица объекта в world-space.
uniform dmat4 uModel;

// Общая матрица вида и проекции.
uniform dmat4 uViewProj;

out vec3 vWorldNormal;

void main()
{
    dvec4 vWorldPosition = uModel * dvec4(aPosition, 1.0);

    dmat3 mNormalMatrix = transpose(inverse(dmat3(uModel)));

    vWorldNormal = normalize(vec3(mNormalMatrix * dvec3(aNormal)));

    dvec4 vClipPosition = uViewProj * vWorldPosition;

    gl_Position = vec4(vClipPosition);
}