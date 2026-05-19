#pragma once

#include "GridRenderer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>

// Номер binding point для общего view uniform buffer.
//
// Этот binding должен совпадать с layout(binding = 0)
// в shaders/common/ViewUniformBlock.glsl.
constexpr GLuint ViewUniformBufferBindingPoint = 0;

// GPU-структура общих данных viewport/camera.
struct SViewUniformData
{
	// View matrix.
	glm::mat4 mView;

	// Projection matrix.
	glm::mat4 mProjection;

	// Inverse view matrix.
	glm::mat4 mInvView;

	// Inverse projection matrix.
	glm::mat4 mInvProjection;

	// Projection * View.
	glm::mat4 mViewProj;

	// Inverse projection * view.
	glm::mat4 mInvViewProj;

	// x = viewport width,
	// y = viewport height,
	// z = 1 / viewport width,
	// w = 1 / viewport height.
	glm::vec4 vViewportSize;

	// xyz = camera position in world-space,
	// w   = unused.
	glm::vec4 vCameraPosition;

	// x = near plane,
	// y = far plane,
	// z = unused,
	// w = unused.
	glm::vec4 vZRange;

	// x = 1.0 для ортографической проекции, 0.0 для перспективной.
	// yzw = unused.
	glm::vec4 vProjectionFlags;
};

// Общий uniform buffer для данных текущего viewport/camera.
class CViewUniformBuffer
{
public:
	CViewUniformBuffer();
	~CViewUniformBuffer();

	CViewUniformBuffer(const CViewUniformBuffer&) = delete;
	CViewUniformBuffer& operator=(const CViewUniformBuffer&) = delete;

	CViewUniformBuffer(CViewUniformBuffer&& other) noexcept;
	CViewUniformBuffer& operator=(CViewUniformBuffer&& other) noexcept;

	// Создает OpenGL buffer
	void Initialize();

	// Освобождает OpenGL buffer
	void Destroy();

	// Обновляет данные буфера
	void Update(const SViewUniformData& sData) const;

	// Привязывает буфер к общему binding point
	void Bind() const;

	// Возвращает OpenGL buffer id.
	GLuint GetBufferId() const;

private:
	// OpenGL UBO id
	GLuint m_nBufferId;
};

// Создает GPU-структуру view uniform data из
// CPU-структуры SViewData
SViewUniformData CreateViewUniformData(
	const SViewData& sViewData,
	const glm::dvec3& vCameraPosition,
	double dNearPlane,
	double dFarPlane
);
