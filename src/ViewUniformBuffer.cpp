#include "ViewUniformBuffer.h"

CViewUniformBuffer::CViewUniformBuffer()
	: m_nBufferId(0)
{
}

CViewUniformBuffer::~CViewUniformBuffer()
{
	Destroy();
}

CViewUniformBuffer::CViewUniformBuffer(CViewUniformBuffer&& other) noexcept
	: m_nBufferId(other.m_nBufferId)
{
	other.m_nBufferId = 0;
}

CViewUniformBuffer& CViewUniformBuffer::operator=(CViewUniformBuffer&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_nBufferId = other.m_nBufferId;
		other.m_nBufferId = 0;
	}

	return *this;
}

void CViewUniformBuffer::Initialize()
{
	if (m_nBufferId != 0)
	{
		return;
	}

	glGenBuffers(1, &m_nBufferId);

	glBindBuffer(GL_UNIFORM_BUFFER, m_nBufferId);

	glBufferData(
		GL_UNIFORM_BUFFER,
		static_cast<GLsizeiptr>(sizeof(SViewUniformData)),
		nullptr,
		GL_DYNAMIC_DRAW
	);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	Bind();
}

void CViewUniformBuffer::Destroy()
{
	if (m_nBufferId != 0)
	{
		glDeleteBuffers(1, &m_nBufferId);
		m_nBufferId = 0;
	}
}

void CViewUniformBuffer::Update(const SViewUniformData& sData) const
{
	if (m_nBufferId == 0)
	{
		return;
	}

	glBindBuffer(GL_UNIFORM_BUFFER, m_nBufferId);

	glBufferSubData(
		GL_UNIFORM_BUFFER,
		0,
		static_cast<GLsizeiptr>(sizeof(SViewUniformData)),
		&sData
	);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void CViewUniformBuffer::Bind() const
{
	if (m_nBufferId == 0)
	{
		return;
	}

	glBindBufferBase(
		GL_UNIFORM_BUFFER,
		ViewUniformBufferBindingPoint,
		m_nBufferId
	);
}

GLuint CViewUniformBuffer::GetBufferId() const
{
	return m_nBufferId;
}

SViewUniformData CreateViewUniformData(
	const SViewData& sViewData,
	const glm::dvec3& vCameraPosition,
	double dNearPlane,
	double dFarPlane
)
{
	SViewUniformData sData;

	sData.mView =
		glm::mat4(sViewData.mView);

	sData.mProjection =
		glm::mat4(sViewData.mProjection);

	sData.mInvView =
		glm::mat4(glm::inverse(sViewData.mView));

	sData.mInvProjection =
		glm::mat4(sViewData.mInvProjection);

	sData.mViewProj =
		glm::mat4(sViewData.mViewProj);

	sData.mInvViewProj =
		glm::mat4(glm::inverse(sViewData.mViewProj));

	const float fViewportWidth =
		static_cast<float>(sViewData.vViewportSize.x);

	const float fViewportHeight =
		static_cast<float>(sViewData.vViewportSize.y);

	sData.vViewportSize = glm::vec4(
		fViewportWidth,
		fViewportHeight,
		fViewportWidth > 0.0f ? 1.0f / fViewportWidth : 0.0f,
		fViewportHeight > 0.0f ? 1.0f / fViewportHeight : 0.0f
	);

	sData.vCameraPosition = glm::vec4(
		static_cast<float>(vCameraPosition.x),
		static_cast<float>(vCameraPosition.y),
		static_cast<float>(vCameraPosition.z),
		0.0f
	);

	sData.vZRange = glm::vec4(
		static_cast<float>(dNearPlane),
		static_cast<float>(dFarPlane),
		0.0f,
		0.0f
	);

	sData.vProjectionFlags = glm::vec4(
		sViewData.bIsOrthographicProjection ? 1.0f : 0.0f,
		0.0f,
		0.0f,
		0.0f
	);

	return sData;
}