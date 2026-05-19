#include "GridRenderer.h"

#include <algorithm>
#include <cmath>

namespace
{
	glm::dvec3 SafeNormalize(
		const glm::dvec3& vVector,
		const glm::dvec3& vFallback
	)
	{
		const double dLength =
			glm::length(vVector);

		if (dLength <= 1e-12)
		{
			return vFallback;
		}

		return vVector / dLength;
	}

	bool ProjectWorldPointToScreen(
		const glm::dmat4& mViewProj,
		const glm::dvec3& vWorldPosition,
		const glm::dvec2& vViewportSize,
		glm::dvec2& vScreenPosition
	)
	{
		const glm::dvec4 vClip =
			mViewProj * glm::dvec4(vWorldPosition, 1.0);

		if (std::abs(vClip.w) < 1e-12)
		{
			return false;
		}

		const glm::dvec3 vNdc =
			glm::dvec3(vClip) / vClip.w;

		vScreenPosition = glm::dvec2(
			(vNdc.x * 0.5 + 0.5) * vViewportSize.x,
			(vNdc.y * 0.5 + 0.5) * vViewportSize.y
		);

		return true;
	}

	double ComputePixelsPerWorldUnit(
		const glm::dmat4& mViewProj,
		const glm::dvec3& vOrigin,
		const glm::dvec3& vDirection,
		const glm::dvec2& vViewportSize
	)
	{
		glm::dvec2 vScreenOrigin(0.0);
		glm::dvec2 vScreenUnit(0.0);

		const glm::dvec3 vNormalizedDirection =
			SafeNormalize(vDirection, glm::dvec3(1.0, 0.0, 0.0));

		const bool bOriginProjected = ProjectWorldPointToScreen(
			mViewProj,
			vOrigin,
			vViewportSize,
			vScreenOrigin
		);

		const bool bUnitProjected = ProjectWorldPointToScreen(
			mViewProj,
			vOrigin + vNormalizedDirection,
			vViewportSize,
			vScreenUnit
		);

		if (!bOriginProjected || !bUnitProjected)
		{
			return 0.0;
		}

		return glm::length(vScreenUnit - vScreenOrigin);
	}

	double RoundUpToNiceStep(double dDesiredStep)
	{
		if (!std::isfinite(dDesiredStep) || dDesiredStep <= 0.0)
		{
			return 1.0;
		}

		const double dExponent =
			std::floor(std::log10(dDesiredStep));

		const double dBase =
			std::pow(10.0, dExponent);

		const double dMantissa =
			dDesiredStep / dBase;

		double dNiceMantissa = 10.0;

		if (dMantissa <= 1.0)
		{
			dNiceMantissa = 1.0;
		}
		else if (dMantissa <= 2.0)
		{
			dNiceMantissa = 2.0;
		}
		else if (dMantissa <= 5.0)
		{
			dNiceMantissa = 5.0;
		}

		return dNiceMantissa * dBase;
	}

	bool UnprojectNdcToEye(
		const glm::dmat4& mInvProjection,
		double dNdcX,
		double dNdcY,
		double dNdcZ,
		glm::dvec3& vEyePoint
	)
	{
		glm::dvec4 vEye =
			mInvProjection * glm::dvec4(dNdcX, dNdcY, dNdcZ, 1.0);

		if (std::abs(vEye.w) < 1e-12)
		{
			return false;
		}

		vEye /= vEye.w;
		vEyePoint = glm::dvec3(vEye);

		return true;
	}

	bool TryIntersectCenterRayWithGridPlane(
		const glm::dmat4& mInvProjection,
		const glm::dvec3& vGridOriginEye,
		const glm::dvec3& vGridAxisXEye,
		const glm::dvec3& vGridAxisYEye,
		const glm::dvec3& vGridNormalEye,
		double& dGridX,
		double& dGridY
	)
	{
		glm::dvec3 vNearEye(0.0);
		glm::dvec3 vFarEye(0.0);

		const bool bNearOk = UnprojectNdcToEye(
			mInvProjection,
			0.0,
			0.0,
			-1.0,
			vNearEye
		);

		const bool bFarOk = UnprojectNdcToEye(
			mInvProjection,
			0.0,
			0.0,
			1.0,
			vFarEye
		);

		if (!bNearOk || !bFarOk)
		{
			return false;
		}

		const glm::dvec3 vRayOriginEye =
			vNearEye;

		const glm::dvec3 vRayDirectionEye =
			SafeNormalize(vFarEye - vNearEye, glm::dvec3(0.0, 0.0, -1.0));

		const double dDenom =
			glm::dot(vRayDirectionEye, vGridNormalEye);

		if (std::abs(dDenom) < 1e-12)
		{
			return false;
		}

		const double dT =
			glm::dot(vGridOriginEye - vRayOriginEye, vGridNormalEye) / dDenom;

		const glm::dvec3 vGridPointEye =
			vRayOriginEye + vRayDirectionEye * dT;

		const glm::dvec3 vGridPointFromOriginEye =
			vGridPointEye - vGridOriginEye;

		dGridX =
			glm::dot(vGridPointFromOriginEye, vGridAxisXEye);

		dGridY =
			glm::dot(vGridPointFromOriginEye, vGridAxisYEye);

		return true;
	}
}

CGridRenderer::CGridRenderer()
	: m_nFullscreenVao(0)
{
	m_sGeometry.vOrigin = glm::dvec3(0.0, 0.0, 0.0);
	m_sGeometry.vAxisX = glm::dvec3(1.0, 0.0, 0.0);
	m_sGeometry.vAxisY = glm::dvec3(0.0, 1.0, 0.0);
	m_sGeometry.vNormal = glm::dvec3(0.0, 0.0, 1.0);

	m_sStyle.dMinorStepX = 1.0;
	m_sStyle.dMinorStepY = 1.0;
	m_sStyle.nMajorLineFrequency = 10;

	m_sStyle.fMinorThickness = 0.75f;
	m_sStyle.fMajorThickness = 1.05f;
	m_sStyle.fAxisThickness = 1.6f;
	// На этом этапе основной вариант сглаживания — аппаратный MSAA.
	m_sStyle.fShaderAntialiasWidth = 0.75f;

	m_sStyle.dMinViewNormalDot = 0.005;
	m_sStyle.dSafeDepthEpsilon = 0.000001;

	m_sStyle.bDebugDepthZones = false;
	m_sStyle.bDrawPlane = true;

	m_sStyle.bIsBounded = false;
	m_sStyle.vBounds = glm::dvec4(-25.0, -25.0, 25.0, 25.0);

	m_sStyle.bDrawDots = false;
	m_sStyle.fDotRadius = 2.0f;

	m_sStyle.bUseAdaptiveStep = false;
	m_sStyle.dTargetMinorStepPixels = 28.0;

	m_sStyle.vPlaneColorTop = glm::vec4(0.115f, 0.135f, 0.160f, 0.22f);
	m_sStyle.vMinorColorTop = glm::vec4(0.250f, 0.285f, 0.325f, 0.22f);
	m_sStyle.vMajorColorTop = glm::vec4(0.390f, 0.430f, 0.480f, 0.62f);
	m_sStyle.vXAxisColorTop = glm::vec4(0.860f, 0.170f, 0.180f, 1.00f);
	m_sStyle.vYAxisColorTop = glm::vec4(0.180f, 0.730f, 0.300f, 1.00f);

	m_sStyle.vPlaneColorBottom = glm::vec4(0.090f, 0.105f, 0.125f, 0.18f);
	m_sStyle.vMinorColorBottom = glm::vec4(0.200f, 0.230f, 0.270f, 0.18f);
	m_sStyle.vMajorColorBottom = glm::vec4(0.310f, 0.350f, 0.400f, 0.52f);
	m_sStyle.vXAxisColorBottom = glm::vec4(0.620f, 0.120f, 0.130f, 0.95f);
	m_sStyle.vYAxisColorBottom = glm::vec4(0.130f, 0.560f, 0.230f, 0.95f);
}

CGridRenderer::~CGridRenderer()
{
	Destroy();
}

CGridRenderer::CGridRenderer(CGridRenderer&& other) noexcept
	: m_nFullscreenVao(other.m_nFullscreenVao)
	, m_sGeometry(other.m_sGeometry)
	, m_sStyle(other.m_sStyle)
{
	other.m_nFullscreenVao = 0;
}

CGridRenderer& CGridRenderer::operator=(CGridRenderer&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_nFullscreenVao = other.m_nFullscreenVao;
		m_sGeometry = other.m_sGeometry;
		m_sStyle = other.m_sStyle;

		other.m_nFullscreenVao = 0;
	}

	return *this;
}

void CGridRenderer::Initialize()
{
	if (m_nFullscreenVao != 0)
	{
		return;
	}

	glGenVertexArrays(1, &m_nFullscreenVao);
}

void CGridRenderer::Destroy()
{
	if (m_nFullscreenVao != 0)
	{
		glDeleteVertexArrays(1, &m_nFullscreenVao);
		m_nFullscreenVao = 0;
	}
}

void CGridRenderer::SetGeometry(const SGridGeometry& sGeometry)
{
	m_sGeometry = sGeometry;
}

void CGridRenderer::SetStyle(const SGridStyle& sStyle)
{
	m_sStyle = sStyle;
}

const SGridGeometry& CGridRenderer::GetGeometry() const
{
	return m_sGeometry;
}

const SGridStyle& CGridRenderer::GetStyle() const
{
	return m_sStyle;
}

SGridComputedSteps CGridRenderer::ComputeGridSteps(
	const SViewData& sViewData,
	bool& bShowMinorGrid,
	bool& bShowMajorGrid,
	bool& bShowAxes
) const
{
	constexpr double dMinStep = 1e-9;

	double dMinorStepX =
		std::max(m_sStyle.dMinorStepX, dMinStep);

	double dMinorStepY =
		std::max(m_sStyle.dMinorStepY, dMinStep);

	const double dPixelsPerUnitX = ComputePixelsPerWorldUnit(
		sViewData.mViewProj,
		m_sGeometry.vOrigin,
		m_sGeometry.vAxisX,
		sViewData.vViewportSize
	);

	const double dPixelsPerUnitY = ComputePixelsPerWorldUnit(
		sViewData.mViewProj,
		m_sGeometry.vOrigin,
		m_sGeometry.vAxisY,
		sViewData.vViewportSize
	);

	if (
		m_sStyle.bUseAdaptiveStep &&
		std::isfinite(dPixelsPerUnitX) &&
		std::isfinite(dPixelsPerUnitY) &&
		dPixelsPerUnitX > 1e-9 &&
		dPixelsPerUnitY > 1e-9
		)
	{
		const double dDesiredMinorStepX =
			m_sStyle.dTargetMinorStepPixels / dPixelsPerUnitX;

		const double dDesiredMinorStepY =
			m_sStyle.dTargetMinorStepPixels / dPixelsPerUnitY;

		dMinorStepX =
			RoundUpToNiceStep(dDesiredMinorStepX);

		dMinorStepY =
			RoundUpToNiceStep(dDesiredMinorStepY);
	}

	const int nMajorLineFrequency =
		std::max(m_sStyle.nMajorLineFrequency, 1);

	SGridComputedSteps sSteps;

	sSteps.vMinorStep = glm::dvec2(
		dMinorStepX,
		dMinorStepY
	);

	sSteps.vMajorStep = glm::dvec2(
		dMinorStepX * static_cast<double>(nMajorLineFrequency),
		dMinorStepY * static_cast<double>(nMajorLineFrequency)
	);

	bShowMinorGrid = true;
	bShowMajorGrid = true;
	bShowAxes = true;

	if (
		std::isfinite(dPixelsPerUnitX) &&
		std::isfinite(dPixelsPerUnitY) &&
		dPixelsPerUnitX > 1e-9 &&
		dPixelsPerUnitY > 1e-9
		)
	{
		const double dMinorStepPixelsX =
			sSteps.vMinorStep.x * dPixelsPerUnitX;

		const double dMinorStepPixelsY =
			sSteps.vMinorStep.y * dPixelsPerUnitY;

		const double dMajorStepPixelsX =
			sSteps.vMajorStep.x * dPixelsPerUnitX;

		const double dMajorStepPixelsY =
			sSteps.vMajorStep.y * dPixelsPerUnitY;

		constexpr double dMinMinorStepPixels = 6.0;
		constexpr double dMinMajorStepPixels = 4.0;

		bShowMinorGrid =
			dMinorStepPixelsX >= dMinMinorStepPixels ||
			dMinorStepPixelsY >= dMinMinorStepPixels;

		bShowMajorGrid =
			dMajorStepPixelsX >= dMinMajorStepPixels ||
			dMajorStepPixelsY >= dMinMajorStepPixels;
	}

	return sSteps;
}

bool CGridRenderer::ComputeGridData(
	SGridShaderData& sGridData,
	const SViewData& sViewData
) const
{
	if (
		sViewData.vViewportSize.x <= 0.0 ||
		sViewData.vViewportSize.y <= 0.0
		)
	{
		return false;
	}

	bool bShowMinorGrid = true;
	bool bShowMajorGrid = true;
	bool bShowAxes = true;

	const SGridComputedSteps sSteps = ComputeGridSteps(
		sViewData,
		bShowMinorGrid,
		bShowMajorGrid,
		bShowAxes
	);

	const glm::dvec3 vOriginEye = glm::dvec3(
		sViewData.mView * glm::dvec4(m_sGeometry.vOrigin, 1.0)
	);

	const glm::dvec3 vAxisXEye = SafeNormalize(
		glm::dvec3(sViewData.mView * glm::dvec4(m_sGeometry.vAxisX, 0.0)),
		glm::dvec3(1.0, 0.0, 0.0)
	);

	const glm::dvec3 vAxisYEye = SafeNormalize(
		glm::dvec3(sViewData.mView * glm::dvec4(m_sGeometry.vAxisY, 0.0)),
		glm::dvec3(0.0, 1.0, 0.0)
	);

	const glm::dvec3 vNormalEye = SafeNormalize(
		glm::cross(vAxisXEye, vAxisYEye),
		glm::dvec3(0.0, 0.0, 1.0)
	);

	double dCenterGridX = 0.0;
	double dCenterGridY = 0.0;

	const bool bHasCenterIntersection =
		TryIntersectCenterRayWithGridPlane(
			sViewData.mInvProjection,
			vOriginEye,
			vAxisXEye,
			vAxisYEye,
			vNormalEye,
			dCenterGridX,
			dCenterGridY
		);

	double dAnchorGridX = 0.0;
	double dAnchorGridY = 0.0;

	if (
		bHasCenterIntersection &&
		sSteps.vMajorStep.x > 0.0 &&
		sSteps.vMajorStep.y > 0.0
		)
	{
		dAnchorGridX =
			std::floor(dCenterGridX / sSteps.vMajorStep.x) * sSteps.vMajorStep.x;

		dAnchorGridY =
			std::floor(dCenterGridY / sSteps.vMajorStep.y) * sSteps.vMajorStep.y;
	}

	const glm::dvec3 vPatternOriginEye =
		vOriginEye +
		vAxisXEye * dAnchorGridX +
		vAxisYEye * dAnchorGridY;

	sGridData.vGridOriginEye = vOriginEye;
	sGridData.vGridAxisXEye = vAxisXEye;
	sGridData.vGridAxisYEye = vAxisYEye;
	sGridData.vGridNormalEye = vNormalEye;
	sGridData.vGridPatternOriginEye = vPatternOriginEye;

	sGridData.vMinorStep = sSteps.vMinorStep;
	sGridData.vMajorStep = sSteps.vMajorStep;

	sGridData.dMinViewNormalDot = m_sStyle.dMinViewNormalDot;
	sGridData.dSafeDepthEpsilon = m_sStyle.dSafeDepthEpsilon;

	sGridData.bDebugDepthZones = m_sStyle.bDebugDepthZones;
	sGridData.bDrawPlane = m_sStyle.bDrawPlane;

	sGridData.bIsBounded = m_sStyle.bIsBounded;
	sGridData.vBounds = m_sStyle.vBounds;

	sGridData.bDrawDots = m_sStyle.bDrawDots;

	sGridData.bShowMinorGrid = bShowMinorGrid;
	sGridData.bShowMajorGrid = bShowMajorGrid;
	sGridData.bShowAxes = bShowAxes;

	sGridData.fMinorThickness = m_sStyle.fMinorThickness;
	sGridData.fMajorThickness = m_sStyle.fMajorThickness;
	sGridData.fAxisThickness = m_sStyle.fAxisThickness;
	sGridData.fDotRadius = m_sStyle.fDotRadius;
	sGridData.fShaderAntialiasWidth = m_sStyle.fShaderAntialiasWidth;

	sGridData.vPlaneColorTop = m_sStyle.vPlaneColorTop;
	sGridData.vMinorColorTop = m_sStyle.vMinorColorTop;
	sGridData.vMajorColorTop = m_sStyle.vMajorColorTop;
	sGridData.vXAxisColorTop = m_sStyle.vXAxisColorTop;
	sGridData.vYAxisColorTop = m_sStyle.vYAxisColorTop;

	sGridData.vPlaneColorBottom = m_sStyle.vPlaneColorBottom;
	sGridData.vMinorColorBottom = m_sStyle.vMinorColorBottom;
	sGridData.vMajorColorBottom = m_sStyle.vMajorColorBottom;
	sGridData.vXAxisColorBottom = m_sStyle.vXAxisColorBottom;
	sGridData.vYAxisColorBottom = m_sStyle.vYAxisColorBottom;

	return true;
}

void CGridRenderer::Render(
	const CShaderProgram& shaderProgram,
	const SViewData& sViewData,
	const SGridShaderData& sGridData
) const
{
	(void)sViewData;

	shaderProgram.Use();

	// Данные viewport/view/projection теперь приходят через общий UBO.
	// Передаём только grid-specific uniform'ы.
	shaderProgram.SetUniformVec3d("uGridOriginEye", sGridData.vGridOriginEye);
	shaderProgram.SetUniformVec3d("uGridAxisXEye", sGridData.vGridAxisXEye);
	shaderProgram.SetUniformVec3d("uGridAxisYEye", sGridData.vGridAxisYEye);
	shaderProgram.SetUniformVec3d("uGridNormalEye", sGridData.vGridNormalEye);
	shaderProgram.SetUniformVec3d("uGridPatternOriginEye", sGridData.vGridPatternOriginEye);

	shaderProgram.SetUniform1d("uMinViewNormalDot", sGridData.dMinViewNormalDot);
	shaderProgram.SetUniform1d("uSafeDepthEpsilon", sGridData.dSafeDepthEpsilon);
	shaderProgram.SetUniform1i("uDebugDepthZones", sGridData.bDebugDepthZones ? 1 : 0);

	shaderProgram.SetUniform1i("uDrawPlane", sGridData.bDrawPlane ? 1 : 0);

	shaderProgram.SetUniform1i("uIsBounded", sGridData.bIsBounded ? 1 : 0);
	shaderProgram.SetUniformVec4d("uGridBounds", sGridData.vBounds);

	shaderProgram.SetUniform1i("uDrawDots", sGridData.bDrawDots ? 1 : 0);

	shaderProgram.SetUniform1i("uShowMinorGrid", sGridData.bShowMinorGrid ? 1 : 0);
	shaderProgram.SetUniform1i("uShowMajorGrid", sGridData.bShowMajorGrid ? 1 : 0);
	shaderProgram.SetUniform1i("uShowAxes", sGridData.bShowAxes ? 1 : 0);

	shaderProgram.SetUniformVec2d("uMinorStep", sGridData.vMinorStep);
	shaderProgram.SetUniformVec2d("uMajorStep", sGridData.vMajorStep);

	shaderProgram.SetUniform1f("uMinorThickness", sGridData.fMinorThickness);
	shaderProgram.SetUniform1f("uMajorThickness", sGridData.fMajorThickness);
	shaderProgram.SetUniform1f("uAxisThickness", sGridData.fAxisThickness);
	shaderProgram.SetUniform1f("uDotRadius", sGridData.fDotRadius);
	shaderProgram.SetUniform1f("uShaderAntialiasWidth", sGridData.fShaderAntialiasWidth);

	shaderProgram.SetUniformVec4f("uPlaneColorTop", sGridData.vPlaneColorTop);
	shaderProgram.SetUniformVec4f("uMinorColorTop", sGridData.vMinorColorTop);
	shaderProgram.SetUniformVec4f("uMajorColorTop", sGridData.vMajorColorTop);
	shaderProgram.SetUniformVec4f("uXAxisColorTop", sGridData.vXAxisColorTop);
	shaderProgram.SetUniformVec4f("uYAxisColorTop", sGridData.vYAxisColorTop);

	shaderProgram.SetUniformVec4f("uPlaneColorBottom", sGridData.vPlaneColorBottom);
	shaderProgram.SetUniformVec4f("uMinorColorBottom", sGridData.vMinorColorBottom);
	shaderProgram.SetUniformVec4f("uMajorColorBottom", sGridData.vMajorColorBottom);
	shaderProgram.SetUniformVec4f("uXAxisColorBottom", sGridData.vXAxisColorBottom);
	shaderProgram.SetUniformVec4f("uYAxisColorBottom", sGridData.vYAxisColorBottom);

	glBindVertexArray(m_nFullscreenVao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}