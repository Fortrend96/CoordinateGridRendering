#include "DemoSceneRenderer.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <stdexcept>

CDemoSceneRenderer::CDemoSceneRenderer()
    : m_bIsInitialized(false)
{
}

CDemoSceneRenderer::~CDemoSceneRenderer()
{
    Destroy();
}

void CDemoSceneRenderer::Initialize()
{
    if (m_bIsInitialized)
    {
        return;
    }

    m_arrMeshes.clear();
    m_arrInstances.clear();

    // Mesh 0: cube.
    {
        std::vector<SVertexPN> arrVertices;
        std::vector<unsigned int> arrIndices;

        BuildUnitCubeMesh(arrVertices, arrIndices);
        m_arrMeshes.push_back(CreateMeshGpu(arrVertices, arrIndices));
    }

    // Mesh 1: sphere.
    {
        std::vector<SVertexPN> arrVertices;
        std::vector<unsigned int> arrIndices;

        BuildUnitSphereMesh(32, 20, arrVertices, arrIndices);
        m_arrMeshes.push_back(CreateMeshGpu(arrVertices, arrIndices));
    }

    // Mesh 2: cylinder.
    {
        std::vector<SVertexPN> arrVertices;
        std::vector<unsigned int> arrIndices;

        BuildUnitCylinderMesh(32, arrVertices, arrIndices);
        m_arrMeshes.push_back(CreateMeshGpu(arrVertices, arrIndices));
    }

    BuildSceneInstances();

    m_bIsInitialized = true;
}

void CDemoSceneRenderer::Destroy()
{
    for (SMeshGpu& sMeshGpu : m_arrMeshes)
    {
        DestroyMeshGpu(sMeshGpu);
    }

    m_arrMeshes.clear();
    m_arrInstances.clear();

    m_bIsInitialized = false;
}

void CDemoSceneRenderer::Render(
    const CShaderProgram& shaderProgram,
    const SGridFrameData& sFrameData,
    const SGridGeometry& sGridGeometry
) const
{
    if (!m_bIsInitialized)
    {
        return;
    }

    shaderProgram.Use();

    shaderProgram.SetUniformMat4d("uViewProj", sFrameData.mViewProj);

    shaderProgram.SetUniformVec3f(
        "uLightDirectionWorld",
        glm::normalize(glm::vec3(0.35f, 0.55f, 0.75f))
    );

    const glm::dmat4 mGridLocalToWorld =
        BuildGridLocalToWorldMatrix(sGridGeometry);

    for (const SDemoInstance& sInstance : m_arrInstances)
    {
        if (sInstance.nMeshIndex >= m_arrMeshes.size())
        {
            continue;
        }

        const SMeshGpu& sMeshGpu = m_arrMeshes[sInstance.nMeshIndex];

        const glm::dmat4 mModelMatrix =
            mGridLocalToWorld * sInstance.mLocalModelMatrix;

        shaderProgram.SetUniformMat4d(
            "uModel",
            mModelMatrix
        );

        shaderProgram.SetUniformVec3f(
            "uBaseColor",
            sInstance.vBaseColor
        );

        glBindVertexArray(sMeshGpu.nVaoId);

        glDrawElements(
            GL_TRIANGLES,
            sMeshGpu.nIndexCount,
            GL_UNSIGNED_INT,
            nullptr
        );
    }

    glBindVertexArray(0);
}

CDemoSceneRenderer::SMeshGpu CDemoSceneRenderer::CreateMeshGpu(
    const std::vector<SVertexPN>& arrVertices,
    const std::vector<unsigned int>& arrIndices
)
{
    if (arrVertices.empty() || arrIndices.empty())
    {
        throw std::runtime_error("Attempt to create mesh from empty data.");
    }

    SMeshGpu sMeshGpu{};

    glGenVertexArrays(1, &sMeshGpu.nVaoId);
    glGenBuffers(1, &sMeshGpu.nVboId);
    glGenBuffers(1, &sMeshGpu.nEboId);

    glBindVertexArray(sMeshGpu.nVaoId);

    glBindBuffer(GL_ARRAY_BUFFER, sMeshGpu.nVboId);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(arrVertices.size() * sizeof(SVertexPN)),
        arrVertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sMeshGpu.nEboId);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(arrIndices.size() * sizeof(unsigned int)),
        arrIndices.data(),
        GL_STATIC_DRAW
    );

    // Attribute 0: position.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(SVertexPN),
        reinterpret_cast<const void*>(offsetof(SVertexPN, vPosition))
    );

    // Attribute 1: normal.
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(SVertexPN),
        reinterpret_cast<const void*>(offsetof(SVertexPN, vNormal))
    );

    glBindVertexArray(0);

    sMeshGpu.nIndexCount = static_cast<GLsizei>(arrIndices.size());

    return sMeshGpu;
}

void CDemoSceneRenderer::DestroyMeshGpu(SMeshGpu& sMeshGpu)
{
    if (sMeshGpu.nEboId != 0)
    {
        glDeleteBuffers(1, &sMeshGpu.nEboId);
        sMeshGpu.nEboId = 0;
    }

    if (sMeshGpu.nVboId != 0)
    {
        glDeleteBuffers(1, &sMeshGpu.nVboId);
        sMeshGpu.nVboId = 0;
    }

    if (sMeshGpu.nVaoId != 0)
    {
        glDeleteVertexArrays(1, &sMeshGpu.nVaoId);
        sMeshGpu.nVaoId = 0;
    }

    sMeshGpu.nIndexCount = 0;
}

glm::dmat4 CDemoSceneRenderer::BuildGridLocalToWorldMatrix(
    const SGridGeometry& sGridGeometry
)
{
    glm::dmat4 mGridLocalToWorld(1.0);

    mGridLocalToWorld[0] = glm::dvec4(sGridGeometry.vAxisX, 0.0);
    mGridLocalToWorld[1] = glm::dvec4(sGridGeometry.vAxisY, 0.0);
    mGridLocalToWorld[2] = glm::dvec4(sGridGeometry.vNormal, 0.0);
    mGridLocalToWorld[3] = glm::dvec4(sGridGeometry.vOrigin, 1.0);

    return mGridLocalToWorld;
}

void CDemoSceneRenderer::BuildUnitCubeMesh(
    std::vector<SVertexPN>& arrVertices,
    std::vector<unsigned int>& arrIndices
)
{
    arrVertices.clear();
    arrIndices.clear();

    const float fHalfSize = 0.5f;

    const glm::vec3 arrFaceNormals[6] =
    {
        glm::vec3(1.0f,  0.0f,  0.0f),
        glm::vec3(-1.0f,  0.0f,  0.0f),
        glm::vec3(0.0f,  1.0f,  0.0f),
        glm::vec3(0.0f, -1.0f,  0.0f),
        glm::vec3(0.0f,  0.0f,  1.0f),
        glm::vec3(0.0f,  0.0f, -1.0f)
    };

    const glm::vec3 arrFacePositions[6][4] =
    {
        {
            glm::vec3(fHalfSize, -fHalfSize, -fHalfSize),
            glm::vec3(fHalfSize,  fHalfSize, -fHalfSize),
            glm::vec3(fHalfSize,  fHalfSize,  fHalfSize),
            glm::vec3(fHalfSize, -fHalfSize,  fHalfSize)
        },
        {
            glm::vec3(-fHalfSize, -fHalfSize,  fHalfSize),
            glm::vec3(-fHalfSize,  fHalfSize,  fHalfSize),
            glm::vec3(-fHalfSize,  fHalfSize, -fHalfSize),
            glm::vec3(-fHalfSize, -fHalfSize, -fHalfSize)
        },
        {
            glm::vec3(-fHalfSize,  fHalfSize, -fHalfSize),
            glm::vec3(-fHalfSize,  fHalfSize,  fHalfSize),
            glm::vec3(fHalfSize,  fHalfSize,  fHalfSize),
            glm::vec3(fHalfSize,  fHalfSize, -fHalfSize)
        },
        {
            glm::vec3(-fHalfSize, -fHalfSize,  fHalfSize),
            glm::vec3(-fHalfSize, -fHalfSize, -fHalfSize),
            glm::vec3(fHalfSize, -fHalfSize, -fHalfSize),
            glm::vec3(fHalfSize, -fHalfSize,  fHalfSize)
        },
        {
            glm::vec3(-fHalfSize, -fHalfSize,  fHalfSize),
            glm::vec3(fHalfSize, -fHalfSize,  fHalfSize),
            glm::vec3(fHalfSize,  fHalfSize,  fHalfSize),
            glm::vec3(-fHalfSize,  fHalfSize,  fHalfSize)
        },
        {
            glm::vec3(-fHalfSize,  fHalfSize, -fHalfSize),
            glm::vec3(fHalfSize,  fHalfSize, -fHalfSize),
            glm::vec3(fHalfSize, -fHalfSize, -fHalfSize),
            glm::vec3(-fHalfSize, -fHalfSize, -fHalfSize)
        }
    };

    for (int nFaceIndex = 0; nFaceIndex < 6; ++nFaceIndex)
    {
        const unsigned int nBaseIndex =
            static_cast<unsigned int>(arrVertices.size());

        for (int nCornerIndex = 0; nCornerIndex < 4; ++nCornerIndex)
        {
            SVertexPN sVertex{};
            sVertex.vPosition = arrFacePositions[nFaceIndex][nCornerIndex];
            sVertex.vNormal = arrFaceNormals[nFaceIndex];

            arrVertices.push_back(sVertex);
        }

        arrIndices.push_back(nBaseIndex + 0);
        arrIndices.push_back(nBaseIndex + 1);
        arrIndices.push_back(nBaseIndex + 2);

        arrIndices.push_back(nBaseIndex + 0);
        arrIndices.push_back(nBaseIndex + 2);
        arrIndices.push_back(nBaseIndex + 3);
    }
}

void CDemoSceneRenderer::BuildUnitSphereMesh(
    int nSliceCount,
    int nStackCount,
    std::vector<SVertexPN>& arrVertices,
    std::vector<unsigned int>& arrIndices
)
{
    arrVertices.clear();
    arrIndices.clear();

    for (int nStackIndex = 0; nStackIndex <= nStackCount; ++nStackIndex)
    {
        const float fV =
            static_cast<float>(nStackIndex) /
            static_cast<float>(nStackCount);

        const float fPhi = glm::pi<float>() * fV;
        const float fSinPhi = std::sin(fPhi);
        const float fCosPhi = std::cos(fPhi);

        for (int nSliceIndex = 0; nSliceIndex <= nSliceCount; ++nSliceIndex)
        {
            const float fU =
                static_cast<float>(nSliceIndex) /
                static_cast<float>(nSliceCount);

            const float fTheta = glm::two_pi<float>() * fU;
            const float fSinTheta = std::sin(fTheta);
            const float fCosTheta = std::cos(fTheta);

            glm::vec3 vNormal(
                fSinPhi * fCosTheta,
                fSinPhi * fSinTheta,
                fCosPhi
            );

            SVertexPN sVertex{};
            sVertex.vPosition = vNormal;
            sVertex.vNormal = glm::normalize(vNormal);

            arrVertices.push_back(sVertex);
        }
    }

    const int nRowVertexCount = nSliceCount + 1;

    for (int nStackIndex = 0; nStackIndex < nStackCount; ++nStackIndex)
    {
        for (int nSliceIndex = 0; nSliceIndex < nSliceCount; ++nSliceIndex)
        {
            const unsigned int nA =
                static_cast<unsigned int>(
                    nStackIndex * nRowVertexCount + nSliceIndex
                    );

            const unsigned int nB = nA + 1;

            const unsigned int nC =
                static_cast<unsigned int>(
                    (nStackIndex + 1) * nRowVertexCount + nSliceIndex
                    );

            const unsigned int nD = nC + 1;

            arrIndices.push_back(nA);
            arrIndices.push_back(nC);
            arrIndices.push_back(nB);

            arrIndices.push_back(nB);
            arrIndices.push_back(nC);
            arrIndices.push_back(nD);
        }
    }
}

void CDemoSceneRenderer::BuildUnitCylinderMesh(
    int nSideCount,
    std::vector<SVertexPN>& arrVertices,
    std::vector<unsigned int>& arrIndices
)
{
    arrVertices.clear();
    arrIndices.clear();

    const float fHalfHeight = 0.5f;

    // Боковая поверхность.
    {
        const unsigned int nBaseIndex =
            static_cast<unsigned int>(arrVertices.size());

        for (int nSideIndex = 0; nSideIndex <= nSideCount; ++nSideIndex)
        {
            const float fT =
                static_cast<float>(nSideIndex) /
                static_cast<float>(nSideCount);

            const float fAngle = glm::two_pi<float>() * fT;
            const float fCosAngle = std::cos(fAngle);
            const float fSinAngle = std::sin(fAngle);

            const glm::vec3 vNormal(fCosAngle, fSinAngle, 0.0f);

            SVertexPN sBottomVertex{};
            sBottomVertex.vPosition =
                glm::vec3(fCosAngle, fSinAngle, -fHalfHeight);
            sBottomVertex.vNormal = vNormal;
            arrVertices.push_back(sBottomVertex);

            SVertexPN sTopVertex{};
            sTopVertex.vPosition =
                glm::vec3(fCosAngle, fSinAngle, fHalfHeight);
            sTopVertex.vNormal = vNormal;
            arrVertices.push_back(sTopVertex);
        }

        for (int nSideIndex = 0; nSideIndex < nSideCount; ++nSideIndex)
        {
            const unsigned int nI0 =
                nBaseIndex + static_cast<unsigned int>(nSideIndex * 2);
            const unsigned int nI1 = nI0 + 1;
            const unsigned int nI2 = nI0 + 2;
            const unsigned int nI3 = nI0 + 3;

            arrIndices.push_back(nI0);
            arrIndices.push_back(nI1);
            arrIndices.push_back(nI2);

            arrIndices.push_back(nI2);
            arrIndices.push_back(nI1);
            arrIndices.push_back(nI3);
        }
    }

    // Верхняя крышка.
    {
        const unsigned int nCenterIndex =
            static_cast<unsigned int>(arrVertices.size());

        SVertexPN sCenterVertex{};
        sCenterVertex.vPosition = glm::vec3(0.0f, 0.0f, fHalfHeight);
        sCenterVertex.vNormal = glm::vec3(0.0f, 0.0f, 1.0f);
        arrVertices.push_back(sCenterVertex);

        const unsigned int nRingStartIndex =
            static_cast<unsigned int>(arrVertices.size());

        for (int nSideIndex = 0; nSideIndex <= nSideCount; ++nSideIndex)
        {
            const float fT =
                static_cast<float>(nSideIndex) /
                static_cast<float>(nSideCount);

            const float fAngle = glm::two_pi<float>() * fT;
            const float fCosAngle = std::cos(fAngle);
            const float fSinAngle = std::sin(fAngle);

            SVertexPN sVertex{};
            sVertex.vPosition = glm::vec3(fCosAngle, fSinAngle, fHalfHeight);
            sVertex.vNormal = glm::vec3(0.0f, 0.0f, 1.0f);
            arrVertices.push_back(sVertex);
        }

        for (int nSideIndex = 0; nSideIndex < nSideCount; ++nSideIndex)
        {
            arrIndices.push_back(nCenterIndex);
            arrIndices.push_back(
                nRingStartIndex + static_cast<unsigned int>(nSideIndex)
            );
            arrIndices.push_back(
                nRingStartIndex + static_cast<unsigned int>(nSideIndex + 1)
            );
        }
    }

    // Нижняя крышка.
    {
        const unsigned int nCenterIndex =
            static_cast<unsigned int>(arrVertices.size());

        SVertexPN sCenterVertex{};
        sCenterVertex.vPosition = glm::vec3(0.0f, 0.0f, -fHalfHeight);
        sCenterVertex.vNormal = glm::vec3(0.0f, 0.0f, -1.0f);
        arrVertices.push_back(sCenterVertex);

        const unsigned int nRingStartIndex =
            static_cast<unsigned int>(arrVertices.size());

        for (int nSideIndex = 0; nSideIndex <= nSideCount; ++nSideIndex)
        {
            const float fT =
                static_cast<float>(nSideIndex) /
                static_cast<float>(nSideCount);

            const float fAngle = glm::two_pi<float>() * fT;
            const float fCosAngle = std::cos(fAngle);
            const float fSinAngle = std::sin(fAngle);

            SVertexPN sVertex{};
            sVertex.vPosition = glm::vec3(fCosAngle, fSinAngle, -fHalfHeight);
            sVertex.vNormal = glm::vec3(0.0f, 0.0f, -1.0f);
            arrVertices.push_back(sVertex);
        }

        for (int nSideIndex = 0; nSideIndex < nSideCount; ++nSideIndex)
        {
            arrIndices.push_back(nCenterIndex);
            arrIndices.push_back(
                nRingStartIndex + static_cast<unsigned int>(nSideIndex + 1)
            );
            arrIndices.push_back(
                nRingStartIndex + static_cast<unsigned int>(nSideIndex)
            );
        }
    }
}

void CDemoSceneRenderer::BuildSceneInstances()
{
    m_arrInstances.clear();

    // Mesh indexes:
    // 0 = cube
    // 1 = sphere
    // 2 = cylinder
    
    // Куб.
    {
        SDemoInstance sInstance{};
        sInstance.nMeshIndex = 0;
        sInstance.vBaseColor = glm::vec3(0.82f, 0.62f, 0.32f);

        glm::dmat4 mLocalModel(1.0);
        mLocalModel = glm::translate(
            mLocalModel,
            glm::dvec3(-4.0, -2.0, 0.0)
        );
        mLocalModel = glm::scale(
            mLocalModel,
            glm::dvec3(1.6, 1.6, 1.6)
        );

        sInstance.mLocalModelMatrix = mLocalModel;
        m_arrInstances.push_back(sInstance);
    }

    // Сфера.
    {
        SDemoInstance sInstance{};
        sInstance.nMeshIndex = 1;
        sInstance.vBaseColor = glm::vec3(0.28f, 0.62f, 0.88f);

        glm::dmat4 mLocalModel(1.0);
        mLocalModel = glm::translate(
            mLocalModel,
            glm::dvec3(0.0, 2.8, 0.0)
        );
        mLocalModel = glm::scale(
            mLocalModel,
            glm::dvec3(1.0, 1.0, 1.0)
        );

        sInstance.mLocalModelMatrix = mLocalModel;
        m_arrInstances.push_back(sInstance);
    }

    // Цилиндр.
    {
        SDemoInstance sInstance{};
        sInstance.nMeshIndex = 2;
        sInstance.vBaseColor = glm::vec3(0.48f, 0.82f, 0.42f);

        glm::dmat4 mLocalModel(1.0);
        mLocalModel = glm::translate(
            mLocalModel,
            glm::dvec3(4.0, -1.5, 0.0)
        );
        mLocalModel = glm::scale(
            mLocalModel,
            glm::dvec3(0.9, 0.9, 2.0)
        );

        sInstance.mLocalModelMatrix = mLocalModel;
        m_arrInstances.push_back(sInstance);
    }
}