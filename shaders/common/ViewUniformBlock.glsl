#ifndef VIEW_UNIFORM_BLOCK_GLSL
#define VIEW_UNIFORM_BLOCK_GLSL

layout(std140, binding = 0) uniform ViewUniformBlock
{
	mat4 mView;
	mat4 mProjection;
	mat4 mInvView;
	mat4 mInvProjection;
	mat4 mViewProj;
	mat4 mInvViewProj;

	vec4 vViewportSize;
	vec4 vCameraPosition;
	vec4 vZRange;
	vec4 vProjectionFlags;
} uViewData;

#define uViewMatrix                  (uViewData.mView)
#define uProjectionMatrix            (uViewData.mProjection)
#define uInvViewMatrix               (uViewData.mInvView)
#define uInvProjectionMatrix         (uViewData.mInvProjection)
#define uViewProjMatrix              (uViewData.mViewProj)
#define uInvViewProjMatrix           (uViewData.mInvViewProj)

#define uViewportWidth               (uViewData.vViewportSize.x)
#define uViewportHeight              (uViewData.vViewportSize.y)
#define uInvViewportWidth            (uViewData.vViewportSize.z)
#define uInvViewportHeight           (uViewData.vViewportSize.w)

#define uCameraWorldPosition         (uViewData.vCameraPosition.xyz)

#define uNearPlane                   (uViewData.vZRange.x)
#define uFarPlane                    (uViewData.vZRange.y)

#define uIsOrthographicProjection    (uViewData.vProjectionFlags.x > 0.5)

#endif