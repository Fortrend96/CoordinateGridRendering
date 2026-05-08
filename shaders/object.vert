#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

// Матрица объекта в world-space.
//
// Используем dmat4, потому что C++ передаёт эту матрицу через
// glUniformMatrix4dv / SetUniformMat4d.
//
// Это также полезно для тестов с большими координатами.
uniform dmat4 uModel;

// Общая матрица вида и проекции.
//
// Используем dmat4, потому что C++ передаёт sFrameData.mViewProj
// через SetUniformMat4d.
uniform dmat4 uViewProj;

out vec3 vWorldNormal;

void main()
{
    // Позицию считаем в double до финального gl_Position.
    //
    // Это важно для large offset preset'ов, где world-space координаты
    // могут быть большими.
    dvec4 vWorldPosition = uModel * dvec4(aPosition, 1.0);

    // Корректная матрица нормалей.
    //
    // Даже если объект будет иметь неравномерный scale,
    // нормали останутся корректными.
    dmat3 mNormalMatrix = transpose(inverse(dmat3(uModel)));

    vWorldNormal = normalize(vec3(mNormalMatrix * dvec3(aNormal)));

    dvec4 vClipPosition = uViewProj * vWorldPosition;

    // gl_Position имеет тип vec4, поэтому финально приводим к float.
    //
    // Все вычисления с большими координатами до этого момента уже сделаны
    // в double.
    gl_Position = vec4(vClipPosition);
}