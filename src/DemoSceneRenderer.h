#pragma once

#include "GridRenderer.h"
#include "ShaderProgram.h"

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

// Рендерер простой тестовой сцены.
//
// Нужен не как часть сетки, а как отдельный тестовый стенд:
// - рисует несколько модельных объектов;
// - позволяет проверить, как сетка взаимодействует с depth buffer;
// - помогает увидеть, корректно ли сетка перекрывается объектами.
//
// Важно:
// объекты рисуются относительно текущей геометрии сетки,
// поэтому они остаются рядом с сеткой даже для rotated / large offset presets.
class CDemoSceneRenderer
{
public:
    // Создаёт пустой рендерер сцены.
    CDemoSceneRenderer();

    // Освобождает ресурсы OpenGL.
    ~CDemoSceneRenderer();

    // Копирование запрещено, потому что класс владеет OpenGL VAO/VBO/EBO.
    CDemoSceneRenderer(const CDemoSceneRenderer&) = delete;

    // Копирующее присваивание запрещено, потому что класс владеет OpenGL VAO/VBO/EBO.
    CDemoSceneRenderer& operator=(const CDemoSceneRenderer&) = delete;

    // Инициализирует GPU-ресурсы и создаёт тестовую сцену.
    void Initialize();

    // Освобождает GPU-ресурсы.
    void Destroy();

    // Рисует все объекты сцены.
    //
    // shaderProgram — шейдер для объектов.
    // sFrameData — данные текущего кадра.
    // sGridGeometry — текущая геометрия сетки.
    void Render(
        const CShaderProgram& shaderProgram,
        const SGridFrameData& sFrameData,
        const SGridGeometry& sGridGeometry
    ) const;

private:
    // Вершина с позицией и нормалью.
    struct SVertexPN
    {
        // Позиция вершины в локальной системе координат меша.
        glm::vec3 vPosition;

        // Нормаль вершины в локальной системе координат меша.
        glm::vec3 vNormal;
    };

    // GPU-представление меша.
    struct SMeshGpu
    {
        // Vertex Array Object.
        GLuint nVaoId = 0;

        // Vertex Buffer Object.
        GLuint nVboId = 0;

        // Element Buffer Object.
        GLuint nEboId = 0;

        // Количество индексов для glDrawElements.
        GLsizei nIndexCount = 0;
    };

    // Экземпляр объекта сцены.
    struct SDemoInstance
    {
        // Индекс меша в массиве m_arrMeshes.
        std::size_t nMeshIndex = 0;

        // Локальная модельная матрица относительно СК сетки.
        //
        // То есть координаты объекта задаются в системе:
        // X = ось сетки X,
        // Y = ось сетки Y,
        // Z = нормаль сетки.
        glm::dmat4 mLocalModelMatrix = glm::dmat4(1.0);

        // Базовый цвет объекта.
        glm::vec3 vBaseColor = glm::vec3(1.0f);
    };

private:
    // Создаёт VAO/VBO/EBO и загружает меш на GPU.
    static SMeshGpu CreateMeshGpu(
        const std::vector<SVertexPN>& arrVertices,
        const std::vector<unsigned int>& arrIndices
    );

    // Уничтожает GPU-ресурсы одного меша.
    static void DestroyMeshGpu(SMeshGpu& sMeshGpu);

    // Строит матрицу перехода из локальной СК сетки в world-space.
    static glm::dmat4 BuildGridLocalToWorldMatrix(
        const SGridGeometry& sGridGeometry
    );

    // Заполняет массивы вершинами и индексами единичного куба.
    static void BuildUnitCubeMesh(
        std::vector<SVertexPN>& arrVertices,
        std::vector<unsigned int>& arrIndices
    );

    // Заполняет массивы вершинами и индексами единичной UV-сферы.
    static void BuildUnitSphereMesh(
        int nSliceCount,
        int nStackCount,
        std::vector<SVertexPN>& arrVertices,
        std::vector<unsigned int>& arrIndices
    );

    // Заполняет массивы вершинами и индексами единичного цилиндра.
    static void BuildUnitCylinderMesh(
        int nSideCount,
        std::vector<SVertexPN>& arrVertices,
        std::vector<unsigned int>& arrIndices
    );

    // Создаёт содержимое тестовой сцены:
    // куб, шар и цилиндр.
    void BuildSceneInstances();

private:
    // Флаг, что рендерер уже инициализирован.
    bool m_bIsInitialized;

    // Набор мешей, которые используются объектами сцены.
    std::vector<SMeshGpu> m_arrMeshes;

    // Экземпляры объектов сцены.
    std::vector<SDemoInstance> m_arrInstances;
};