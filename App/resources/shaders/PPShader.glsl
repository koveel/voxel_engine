#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;

void main()
{
	gl_Position = vec4(a_Position, 0.0f, 1.0f);
}

#type fragment
#version 450 core

layout(location = 3) out float o_AOAccum;

uniform vec2 u_ViewportDims;

in vec2 o_UV;

layout(binding = 2) uniform sampler2D u_PreviousDepth;
layout(binding = 3) uniform sampler2D u_Albedo;
layout(binding = 4) uniform sampler2D u_Normal;
layout(binding = 5) uniform sampler2D u_PreviousNormal;
layout(binding = 6) uniform sampler2D u_AmbientAccumulation;
layout(binding = 7) uniform sampler2D u_Lighting;

#include "raytracing.glinc"

uniform mat4 u_ViewProjection;
uniform mat4 u_PrevFrameViewProjection;

const int AmbientOcclusionRaysMaxDistanceMeters = 24;
float CastAmbientOcclusionRay(vec3 origin, vec3 direction)
{
	vec3 rayOrigin = origin;
	float totalTraveled = 0.0f;
	float t;

	for (int lod = 0; lod < 3; lod++) {
		rayOrigin = origin + direction * ((totalTraveled + float(lod == 0)) * 1.01f * g_ShadowLODScales[lod]); // brless first mip offset

		bool hit = RaycastShadowMapVariableFidelity(rayOrigin, direction, t, g_ShadowLODDistances[lod], lod);
		totalTraveled += t;
	
		if (hit) {
			return clamp(totalTraveled / 24.0, 0.0, 1.0);
		}
	}
	
	return 1.0f; // no hit br
}

vec2 GetLastFrameUVFromThisFrameWorldPos(vec3 worldPos)
{
	vec4 prevClip = u_PrevFrameViewProjection * vec4(worldPos, 1.0f);
	if (prevClip.w <= 0.0f)
		return vec2(-1.0f);
	vec2 prevNdc = prevClip.xy / prevClip.w;
	return prevNdc * 0.5f + 0.5f; // -1,1 -> 0, 1
}

float LinearizeDepth(float depth)
{
	const float near = 0.1f, far = 100.0f;
	float z = depth * 2.0 - 1.0;
	return (2.0 * near * far) / (far + near - z * (far - near));
}

vec2 GetScreenSpaceMotionVector(vec3 worldPos)
{
	vec4 currClip = u_ViewProjection * vec4(worldPos, 1.0);
	currClip /= currClip.w;
	vec2 currUV = currClip.xy * 0.5 + 0.5;

	vec4 prevClip = u_PrevFrameViewProjection * vec4(worldPos, 1.0);
	prevClip /= prevClip.w;
	vec2 prevUV = prevClip.xy * 0.5 + 0.5;

	vec2 motion = currUV - prevUV;

	if (any(isnan(motion)) || any(greaterThan(abs(motion), vec2(1.0))))
		motion = vec2(0.0);

	return motion;
}

void main()
{
	vec2 uv = gl_FragCoord.xy / u_ViewportDims;

	float depth = texture(u_DepthTexture, uv).r;
	if (depth == 1.0f) return;

	vec4 albedo = texture(u_Albedo, uv);
	vec3 normal = texture(u_Normal, uv).xyz;
	vec3 worldSpaceFragment = ReconstructWorldSpaceFromDepth(uv);
	
	float this_frame_ao = 0.0f;
	const int AmbientOcclusionRaysPerPixel = 2;
	for (int i = 0; i < AmbientOcclusionRaysPerPixel; i++)
	{
		vec3 direction = RandomDirectionOnHemisphere(normal, ivec2(gl_FragCoord), u_FrameNumber, i);
		float norm = CastAmbientOcclusionRay(worldSpaceFragment, direction);
		
		this_frame_ao += norm;
	}
	this_frame_ao /= AmbientOcclusionRaysPerPixel;

	vec2 previousUV = GetLastFrameUVFromThisFrameWorldPos(worldSpaceFragment);
	bool inBounds = all(greaterThan(previousUV, vec2(0.0f))) && all(lessThan(previousUV, vec2(1.0f)));
	if (!inBounds)
	{
		o_AOAccum = this_frame_ao;
		return;
	}

	vec2 pixelSize = 1.0f / u_ViewportDims;

	// we was just in da neighborhood
	float aoPrev = texture(u_AmbientAccumulation, previousUV).r;
	float ao1 = texture(u_AmbientAccumulation, previousUV + vec2(-pixelSize.x, 0.0f)).r;
	float ao2 = texture(u_AmbientAccumulation, previousUV + vec2(pixelSize.x, 0.0f)).r;
	float ao3 = texture(u_AmbientAccumulation, previousUV + vec2(0.0f, -pixelSize.y)).r;
	float ao4 = texture(u_AmbientAccumulation, previousUV + vec2(0.0f, pixelSize.y)).r;
	
	float mi = min(min(min(min(aoPrev, ao1), ao2), ao3), ao4);
	float ma = max(max(max(max(aoPrev, ao1), ao2), ao3), ao4);
	
	// Clamp current frame to previous frame's neighborhood
	float this_ao = clamp(this_frame_ao, mi, ma);

	// depth
	float oldDepth = texture(u_PreviousDepth, previousUV).r;
	float currentLinearDepth = LinearizeDepth(depth);
	float previousLinearDepth = LinearizeDepth(oldDepth);
	float depthDiff = abs(currentLinearDepth - previousLinearDepth);
	const float depthThresholdMul = 0.2f;
	float depthThreshold = depthThresholdMul * currentLinearDepth; // Scale with depth
	float depthWeight = 1.0f - clamp(depthDiff / depthThreshold, 0.0f, 1.0f);

	// normals
	vec3 oldNormal = texture(u_PreviousNormal, previousUV).xyz;
	float normalWeight = pow(max(0.0f, dot(normal, oldNormal)), 4.0f);
	
	// Motion
	float motionLen = length(GetScreenSpaceMotionVector(worldSpaceFragment));
	//float velocityAlpha = smoothstep(0.0f, 0.02f, motionLen);
	float velocityAlpha = clamp(motionLen * 50.0f, 0.0f, 1.0f);
	
	float historyConfidence = depthWeight * normalWeight;
	
	// blend factor
	const float baseAlpha = 0.05f, maxAlpha = 0.95f;
	float alpha = mix(maxAlpha, baseAlpha, clamp(historyConfidence, 0.0f, 1.0f));
	
	// motion discards history
	alpha = max(alpha, velocityAlpha);

	o_AOAccum = mix(aoPrev, this_frame_ao, alpha);
	//o_AOAccum = this_frame_ao;
}