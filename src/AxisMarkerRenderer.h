#pragma once

#include "GridRenderer.h"
#include "ShaderProgram.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

// Визуальные параметры маркера начала системы координат.
//
// Маркер работает в  режимах 2D и 3D.
// Все размеры задаются в пикселях, чтобы маркер не менял визуальный размер
// при приближении и отдалении камеры.
struct SAxisMarkerStyle
{
    // Длина короткой оси маркера в пикселях.
    double dAxisLengthPixels;

    // Отступ буквы от конца оси в пикселях.
    double dLetterOffsetPixels;

    // Половинный размер букв X/Y/Z в пикселях.
    double dLetterSizePixels;

    // Половинный размер квадрата в origin для orthographic-режима.
    double dOriginSquareHalfSizePixels;

    // Толщина линий маркера.
    float fLineWidth;

    // Цвет линий осей маркера.
    glm::vec4 vAxisColor;

    // Цвет букв X/Y/Z.
    glm::vec4 vTextColor;

    // Цвет маленького квадрата в origin.
    glm::vec4 vOriginSquareColor;
};

// Renderer маркера начала системы координат.
//
// Маркер рисуется отдельным проходом поверх сетки.
class CAxisMarkerRenderer
{
public:
    // Создаёт renderer с дефолтным стилем.
    CAxisMarkerRenderer();

    // Освобождает OpenGL-ресурсы renderer'а.
    ~CAxisMarkerRenderer();

    // Копирование и копирующее присваивание запрещено, потому что renderer владеет OpenGL VAO/VBO.
    CAxisMarkerRenderer(const CAxisMarkerRenderer&) = delete;
    CAxisMarkerRenderer& operator=(const CAxisMarkerRenderer&) = delete;

    // Перемещающий конструктор передаёт владение OpenGL VAO/VBO.
    CAxisMarkerRenderer(CAxisMarkerRenderer&& other) noexcept;

    // Перемещающее присваивание передаёт владение OpenGL VAO/VBO.
    CAxisMarkerRenderer& operator=(CAxisMarkerRenderer&& other) noexcept;

    // Создаёт VAO/VBO для line geometry маркера.
    void Initialize();

    // Удаляет VAO/VBO, если они были созданы.
    void Destroy();

    // Задаёт визуальный стиль маркера.
    void SetStyle(const SAxisMarkerStyle& sStyle);

    // Возвращает текущий визуальный стиль маркера.
    const SAxisMarkerStyle& GetStyle() const;

    // Рисует маркер начала системы координат.
    void Render(
        const CShaderProgram& shaderProgram,
        const SViewData& sFrameData,
        const SGridGeometry& sGridGeometry
    ) const;

private:
    // Рисует perspective-вариант маркера.
    void RenderPerspectiveMarker(
        const CShaderProgram& shaderProgram,
        const SViewData& sFrameData,
        const SGridGeometry& sGridGeometry
    ) const;

    // Рисует orthographic-вариант маркера.
    void RenderOrthographicMarker(
        const CShaderProgram& shaderProgram,
        const SViewData& sFrameData,
        const SGridGeometry& sGridGeometry
    ) const;

    // Загружает подготовленные вершины в VBO и рисует их как GL_LINES.
    void UploadAndDrawLines(
        const CShaderProgram& shaderProgram,
        const SViewData& sFrameData,
        const SGridGeometry& sGridGeometry,
        const void* pVertexData,
        GLsizeiptr nVertexDataSize,
        GLsizei nVertexCount
    ) const;

private:
    // VAO для line geometry маркера.
    GLuint m_nVao;

    // Dynamic VBO для line geometry маркера.
    GLuint m_nVbo;

    // Текущий визуальный стиль маркера.
    SAxisMarkerStyle m_sStyle;
};