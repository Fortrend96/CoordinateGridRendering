#version 430 core

layout(location = 0) out vec4 outColor;

// Frame data
uniform dmat4 uViewProj;
uniform dmat4 uInvViewProj;
uniform dvec2 uViewportSize;

// Grid geometry
uniform dvec3 uGridOrigin;
uniform dvec3 uGridAxisX;
uniform dvec3 uGridAxisY;
uniform dvec3 uGridNormal;

uniform double uMinViewNormalDot;

// Grid depth / debug
uniform double uSafeDepthEpsilon;
uniform bool uDebugDepthZones;

// Grid modes
uniform bool uIsBounded;
uniform dvec4 uGridBounds;

uniform bool uDrawDots;

uniform bool uShowMinorGrid;
uniform bool uShowMajorGrid;
uniform bool uShowAxes;

// Grid step / thickness
uniform double uMinorStep;
uniform double uMajorStep;

uniform float uMinorThickness;
uniform float uMajorThickness;
uniform float uAxisThickness;
uniform float uDotRadius;

// Colors
uniform vec4 uMinorColorTop;
uniform vec4 uMajorColorTop;
uniform vec4 uXAxisColorTop;
uniform vec4 uYAxisColorTop;

uniform vec4 uMinorColorBottom;
uniform vec4 uMajorColorBottom;
uniform vec4 uXAxisColorBottom;
uniform vec4 uYAxisColorBottom;

// Helpers
double DistanceToGridLine(double dCoordinate, double dStep)
{
    const double dNearestLine = round(dCoordinate / dStep) * dStep;
    return abs(dCoordinate - dNearestLine);
}

float CreateLineMask(
    double dDistanceToLine,
    float fCoordinatePixelWidth,
    float fThicknessPixels
)
{
    const float fDistancePixels =
        float(dDistanceToLine) / max(fCoordinatePixelWidth, 1e-6);

    const float fHalfThickness = max(fThicknessPixels * 0.5, 0.35);
    const float fAntiAliasWidth = 0.75;

    return 1.0 - smoothstep(
        fHalfThickness,
        fHalfThickness + fAntiAliasWidth,
        fDistancePixels
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

    const float fDistance = length(vDistancePixels);

    return 1.0 - smoothstep(
        fRadiusPixels,
        fRadiusPixels + 1.0,
        fDistance
    );
}

void main()
{
    // Восстанавливаем луч текущего пикселя из gl_FragCoord.
    const dvec2 vNdc = dvec2(
        double(gl_FragCoord.x) / uViewportSize.x * 2.0 - 1.0,
        double(gl_FragCoord.y) / uViewportSize.y * 2.0 - 1.0
    );

    const dvec4 vNearClip = dvec4(vNdc.x, vNdc.y, -1.0, 1.0);
    const dvec4 vFarClip  = dvec4(vNdc.x, vNdc.y,  1.0, 1.0);

    dvec4 vNearWorld = uInvViewProj * vNearClip;
    dvec4 vFarWorld  = uInvViewProj * vFarClip;

    vNearWorld /= vNearWorld.w;
    vFarWorld  /= vFarWorld.w;

    const dvec3 vRayOriginLocal = vNearWorld.xyz - uGridOrigin;
    const dvec3 vRayFarLocal = vFarWorld.xyz - uGridOrigin;
    const dvec3 vRayDirection = normalize(vRayFarLocal - vRayOriginLocal);

    const double dDenom = dot(vRayDirection, uGridNormal);

    if (abs(dDenom) < uMinViewNormalDot)
    {
        discard;
    }

    // Пересечение луча с плоскостью сетки.
    const double dT = -dot(vRayOriginLocal, uGridNormal) / dDenom;
    const dvec3 vLocalPosition = vRayOriginLocal + vRayDirection * dT;

    const double dGridX = dot(vLocalPosition, uGridAxisX);
    const double dGridY = dot(vLocalPosition, uGridAxisY);

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

    const double dMinorDistanceX = DistanceToGridLine(dGridX, uMinorStep);
    const double dMinorDistanceY = DistanceToGridLine(dGridY, uMinorStep);

    const double dMajorDistanceX = DistanceToGridLine(dGridX, uMajorStep);
    const double dMajorDistanceY = DistanceToGridLine(dGridY, uMajorStep);

    const float fGridX = float(dGridX);
    const float fGridY = float(dGridY);

    const float fPixelWidthX = max(fwidth(fGridX), 1e-6);
    const float fPixelWidthY = max(fwidth(fGridY), 1e-6);

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

            fMinorMask = max(fMinorX, fMinorY);
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

            fMajorMask = max(fMajorX, fMajorY);
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

    const bool bTopSide = dDenom < 0.0;

    const vec4 vMinorColor = bTopSide ? uMinorColorTop : uMinorColorBottom;
    const vec4 vMajorColor = bTopSide ? uMajorColorTop : uMajorColorBottom;
    const vec4 vXAxisColor = bTopSide ? uXAxisColorTop : uXAxisColorBottom;
    const vec4 vYAxisColor = bTopSide ? uYAxisColorTop : uYAxisColorBottom;

    const vec4 vBoundaryColor = vMajorColor;

    const float fGridMask = max(
        max(fMinorMask, fMajorMask),
        max(max(fBoundaryMask, fXAxisMask), fYAxisMask)
    );

    if (fGridMask <= 0.001 && !uDebugDepthZones)
    {
        discard;
    }

    vec4 vColor = vec4(0.0);

    vColor = mix(vColor, vMinorColor, fMinorMask);
    vColor = mix(vColor, vMajorColor, fMajorMask);
    vColor = mix(vColor, vBoundaryColor, fBoundaryMask);
    vColor = mix(vColor, vXAxisColor, fXAxisMask);
    vColor = mix(vColor, vYAxisColor, fYAxisMask);

    if (vColor.a <= 0.001 && !uDebugDepthZones)
    {
        discard;
    }

    // Считаем глубину точки сетки и всегда прижимаем её к безопасному диапазону.
    const dvec3 vWorldPosition = uGridOrigin + vLocalPosition;
    const dvec4 vClipPosition = uViewProj * dvec4(vWorldPosition, 1.0);

    const double dNdcZ = vClipPosition.z / vClipPosition.w;
    const double dRawDepth = dNdcZ * 0.5 + 0.5;

    double dSafeMinDepth = clamp(uSafeDepthEpsilon, 0.0, 0.499999);
    double dSafeMaxDepth = clamp(1.0 - uSafeDepthEpsilon, 0.500001, 1.0);

    const bool bNearDepthZone = dRawDepth < dSafeMinDepth;
    const bool bFarDepthZone = dRawDepth > dSafeMaxDepth;

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