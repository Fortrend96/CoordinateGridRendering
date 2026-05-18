#version 430 core

layout(location = 0) out vec4 outColor;

uniform dmat4 uProjection;
uniform dmat4 uInvProjection;
uniform dvec2 uViewportSize;

uniform dvec3 uGridOriginEye;
uniform dvec3 uGridAxisXEye;
uniform dvec3 uGridAxisYEye;
uniform dvec3 uGridNormalEye;

// Anchor для построения pattern'а сетки.
// Он находится около текущей видимой области и привязан к major step.
uniform dvec3 uGridPatternOriginEye;

uniform double uMinViewNormalDot;
uniform double uSafeDepthEpsilon;
uniform bool uDebugDepthZones;

uniform bool uDrawPlane;

uniform bool uIsBounded;
uniform dvec4 uGridBounds;

uniform bool uDrawDots;

uniform bool uShowMinorGrid;
uniform bool uShowMajorGrid;
uniform bool uShowAxes;

// Готовые шаги сетки.
uniform dvec2 uMinorStep;
uniform dvec2 uMajorStep;

uniform float uMinorThickness;
uniform float uMajorThickness;
uniform float uAxisThickness;
uniform float uDotRadius;

uniform float uShaderAntialiasWidth;

uniform vec4 uPlaneColorTop;
uniform vec4 uMinorColorTop;
uniform vec4 uMajorColorTop;
uniform vec4 uXAxisColorTop;
uniform vec4 uYAxisColorTop;

uniform vec4 uPlaneColorBottom;
uniform vec4 uMinorColorBottom;
uniform vec4 uMajorColorBottom;
uniform vec4 uXAxisColorBottom;
uniform vec4 uYAxisColorBottom;

double DistanceToGridLine(
	double dCoordinate,
	double dStep
)
{
	const double dSafeStep =
		max(dStep, 1e-9);

	const double dNearestLine =
		round(dCoordinate / dSafeStep) * dSafeStep;

	return abs(dCoordinate - dNearestLine);
}

float CreateCoverageMask(
	float fDistancePixels,
	float fHalfThicknessPixels
)
{
	if (uShaderAntialiasWidth <= 0.001)
	{
		return fDistancePixels <= fHalfThicknessPixels ? 1.0 : 0.0;
	}

	return 1.0 - smoothstep(
		fHalfThicknessPixels,
		fHalfThicknessPixels + uShaderAntialiasWidth,
		fDistancePixels
	);
}

float CreateLineMask(
	double dDistanceToLine,
	float fCoordinatePixelWidth,
	float fThicknessPixels
)
{
	const float fDistancePixels =
		float(dDistanceToLine) / max(fCoordinatePixelWidth, 1e-6);

	const float fHalfThickness =
		max(fThicknessPixels * 0.5, 0.35);

	return CreateCoverageMask(
		fDistancePixels,
		fHalfThickness
	);
}

float CreateDotMask(
	double dDistanceX,
	double dDistanceY,
	float fPixelWidthX,
	float fPixelWidthY,
	float fRadiusPixels
)
{
	const vec2 vDistancePixels = vec2(
		float(dDistanceX) / max(fPixelWidthX, 1e-6),
		float(dDistanceY) / max(fPixelWidthY, 1e-6)
	);

	const float fDistance =
		length(vDistancePixels);

	if (uShaderAntialiasWidth <= 0.001)
	{
		return fDistance <= fRadiusPixels ? 1.0 : 0.0;
	}

	return 1.0 - smoothstep(
		fRadiusPixels,
		fRadiusPixels + uShaderAntialiasWidth,
		fDistance
	);
}

void main()
{
	// Восстанавливаем NDC текущего пикселя из gl_FragCoord.
	const dvec2 vNdc = dvec2(
		double(gl_FragCoord.x) / uViewportSize.x * 2.0 - 1.0,
		double(gl_FragCoord.y) / uViewportSize.y * 2.0 - 1.0
	);

	const dvec4 vNearClip =
		dvec4(vNdc.x, vNdc.y, -1.0, 1.0);

	const dvec4 vFarClip =
		dvec4(vNdc.x, vNdc.y, 1.0, 1.0);

	dvec4 vNearEye =
		uInvProjection * vNearClip;

	dvec4 vFarEye =
		uInvProjection * vFarClip;

	vNearEye /= vNearEye.w;
	vFarEye /= vFarEye.w;

	// Луч текущего пикселя в eye-space.
	const dvec3 vRayOriginEye =
		vNearEye.xyz;

	const dvec3 vRayDirectionEye =
		normalize(vFarEye.xyz - vNearEye.xyz);

	const double dDenom =
		dot(vRayDirectionEye, uGridNormalEye);

	// Если луч почти параллелен плоскости сетки, скрываем фрагмент.
	if (abs(dDenom) < uMinViewNormalDot)
	{
		discard;
	}

	// Пересечение eye-space луча с eye-space плоскостью сетки.
	const double dT =
		dot(uGridOriginEye - vRayOriginEye, uGridNormalEye) / dDenom;

	const dvec3 vGridPointEye =
		vRayOriginEye + vRayDirectionEye * dT;

	// Абсолютные координаты в системе сетки.
	//
	// Они нужны для:
	// - bounded-режима;
	// - осей X/Y, которые должны быть привязаны к настоящему origin сетки.
	const dvec3 vGridPointFromOriginEye =
		vGridPointEye - uGridOriginEye;

	const double dGridX =
		dot(vGridPointFromOriginEye, uGridAxisXEye);

	const double dGridY =
		dot(vGridPointFromOriginEye, uGridAxisYEye);

	if (uIsBounded)
	{
		if (
			dGridX < uGridBounds.x ||
			dGridY < uGridBounds.y ||
			dGridX > uGridBounds.z ||
			dGridY > uGridBounds.w
		)
		{
			discard;
		}
	}

	// Локальные координаты pattern'а относительно anchor.
	const dvec3 vGridPointFromPatternOriginEye =
		vGridPointEye - uGridPatternOriginEye;

	const double dPatternX =
		dot(vGridPointFromPatternOriginEye, uGridAxisXEye);

	const double dPatternY =
		dot(vGridPointFromPatternOriginEye, uGridAxisYEye);

	// Шаги уже рассчитаны на C++ стороне.
	// Shader только использует готовые значения.
	const double dMinorDistanceX =
		DistanceToGridLine(dPatternX, uMinorStep.x);

	const double dMinorDistanceY =
		DistanceToGridLine(dPatternY, uMinorStep.y);

	const double dMajorDistanceX =
		DistanceToGridLine(dPatternX, uMajorStep.x);

	const double dMajorDistanceY =
		DistanceToGridLine(dPatternY, uMajorStep.y);

	// fwidth считаем от локальных pattern-координат,
	// а не от больших абсолютных gridX/gridY.
	const float fPatternX =
		float(dPatternX);

	const float fPatternY =
		float(dPatternY);

	const float fPixelWidthX =
		max(fwidth(fPatternX), 1e-6);

	const float fPixelWidthY =
		max(fwidth(fPatternY), 1e-6);

	float fMinorMask = 0.0;
	float fMajorMask = 0.0;
	float fBoundaryMask = 0.0;
	float fXAxisMask = 0.0;
	float fYAxisMask = 0.0;

	const double dAxisDirectionEpsilon = 1e-9;

	const float fXAxisDirectionMask =
		dGridX >= -dAxisDirectionEpsilon ? 1.0 : 0.0;

	const float fYAxisDirectionMask =
		dGridY >= -dAxisDirectionEpsilon ? 1.0 : 0.0;

	if (uDrawDots)
	{
		if (uShowMinorGrid)
		{
			fMinorMask = CreateDotMask(
				dMinorDistanceX,
				dMinorDistanceY,
				fPixelWidthX,
				fPixelWidthY,
				uDotRadius
			);
		}

		if (uShowMajorGrid)
		{
			fMajorMask = CreateDotMask(
				dMajorDistanceX,
				dMajorDistanceY,
				fPixelWidthX,
				fPixelWidthY,
				uDotRadius * 1.35
			);
		}

		if (uShowAxes)
		{
			fXAxisMask =
				fXAxisDirectionMask *
				CreateLineMask(abs(dGridY), fPixelWidthY, uAxisThickness);

			fYAxisMask =
				fYAxisDirectionMask *
				CreateLineMask(abs(dGridX), fPixelWidthX, uAxisThickness);
		}
	}
	else
	{
		if (uShowMinorGrid)
		{
			const float fMinorX = CreateLineMask(
				dMinorDistanceX,
				fPixelWidthX,
				uMinorThickness
			);

			const float fMinorY = CreateLineMask(
				dMinorDistanceY,
				fPixelWidthY,
				uMinorThickness
			);

			fMinorMask =
				max(fMinorX, fMinorY);
		}

		if (uShowMajorGrid)
		{
			const float fMajorX = CreateLineMask(
				dMajorDistanceX,
				fPixelWidthX,
				uMajorThickness
			);

			const float fMajorY = CreateLineMask(
				dMajorDistanceY,
				fPixelWidthY,
				uMajorThickness
			);

			fMajorMask =
				max(fMajorX, fMajorY);
		}

		if (uShowAxes)
		{
			fXAxisMask =
				fXAxisDirectionMask *
				CreateLineMask(abs(dGridY), fPixelWidthY, uAxisThickness);

			fYAxisMask =
				fYAxisDirectionMask *
				CreateLineMask(abs(dGridX), fPixelWidthX, uAxisThickness);
		}
	}

	// Рамка bounded-сетки остаётся привязана к абсолютным координатам сетки.
	if (uIsBounded)
	{
		const float fBoundaryMinX = CreateLineMask(
			abs(dGridX - uGridBounds.x),
			fPixelWidthX,
			uMajorThickness
		);

		const float fBoundaryMaxX = CreateLineMask(
			abs(dGridX - uGridBounds.z),
			fPixelWidthX,
			uMajorThickness
		);

		const float fBoundaryMinY = CreateLineMask(
			abs(dGridY - uGridBounds.y),
			fPixelWidthY,
			uMajorThickness
		);

		const float fBoundaryMaxY = CreateLineMask(
			abs(dGridY - uGridBounds.w),
			fPixelWidthY,
			uMajorThickness
		);

		fBoundaryMask = max(
			max(fBoundaryMinX, fBoundaryMaxX),
			max(fBoundaryMinY, fBoundaryMaxY)
		);
	}

	const bool bTopSide =
		dDenom < 0.0;

	const vec4 vPlaneColor =
		bTopSide ? uPlaneColorTop : uPlaneColorBottom;

	const vec4 vMinorColor =
		bTopSide ? uMinorColorTop : uMinorColorBottom;

	const vec4 vMajorColor =
		bTopSide ? uMajorColorTop : uMajorColorBottom;

	const vec4 vXAxisColor =
		bTopSide ? uXAxisColorTop : uXAxisColorBottom;

	const vec4 vYAxisColor =
		bTopSide ? uYAxisColorTop : uYAxisColorBottom;

	const vec4 vBoundaryColor =
		vMajorColor;

	const float fGridMask = max(
		max(fMinorMask, fMajorMask),
		max(max(fBoundaryMask, fXAxisMask), fYAxisMask)
	);

	if (fGridMask <= 0.001 && !uDrawPlane && !uDebugDepthZones)
	{
		discard;
	}

	vec4 vColor =
		uDrawPlane ? vPlaneColor : vec4(0.0);

	vColor = mix(vColor, vMinorColor, fMinorMask);
	vColor = mix(vColor, vMajorColor, fMajorMask);
	vColor = mix(vColor, vBoundaryColor, fBoundaryMask);
	vColor = mix(vColor, vXAxisColor, fXAxisMask);
	vColor = mix(vColor, vYAxisColor, fYAxisMask);

	if (vColor.a <= 0.001 && !uDebugDepthZones)
	{
		discard;
	}

	// Depth считаем из eye-space позиции через projection.
	const dvec4 vClipPosition =
		uProjection * dvec4(vGridPointEye, 1.0);

	const double dNdcZ =
		vClipPosition.z / vClipPosition.w;

	const double dRawDepth =
		dNdcZ * 0.5 + 0.5;

	const double dSafeMinDepth =
		clamp(uSafeDepthEpsilon, 0.0, 0.499999);

	const double dSafeMaxDepth =
		clamp(1.0 - uSafeDepthEpsilon, 0.500001, 1.0);

	const bool bNearDepthZone =
		dRawDepth < dSafeMinDepth;

	const bool bFarDepthZone =
		dRawDepth > dSafeMaxDepth;

	// Ручной depth clamp всегда включён.
	const double dDepth = clamp(
		dRawDepth,
		dSafeMinDepth,
		dSafeMaxDepth
	);

	if (uDebugDepthZones)
	{
		if (bNearDepthZone)
		{
			outColor = vec4(1.0, 0.1, 0.1, 0.85);
		}
		else if (bFarDepthZone)
		{
			outColor = vec4(0.1, 0.25, 1.0, 0.85);
		}
		else
		{
			outColor = vec4(0.1, 0.9, 0.25, 0.65);
		}

		gl_FragDepth = float(dDepth);
		return;
	}

	gl_FragDepth = float(dDepth);
	outColor = vColor;
}