#version 450 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform usampler3D u_ShadowMap;
layout(binding = 1) uniform sampler2D u_BlueNoiseTexture;

layout(binding = 2) uniform sampler2D u_Depth;
layout(binding = 3) uniform sampler2D u_PreviousDepth;

layout(binding = 4) uniform sampler2D u_Normal;
layout(binding = 5) uniform sampler2D u_PreviousNormal;

layout(binding = 6) uniform sampler2D u_AORead;
layout(r8, binding = 0) uniform writeonly image2D u_Output;

uniform mat4 u_InverseView;
uniform mat4 u_ViewProjection;
uniform mat4 u_InverseProjection;
uniform mat4 u_PrevFrameViewProjection;
uniform int u_FrameNumber;

uniform vec3 u_CameraPos;

const float g_BaseVoxelScale = 0.1f;
const float g_ShadowLODScales[3] = float[3](g_BaseVoxelScale, g_BaseVoxelScale * 2.0f, g_BaseVoxelScale * 4.0f);

const int g_AORayTotalDistance = 16;
const int g_AORayDistances[3] = int[3](3, 5, 8);

const float g_PI = 3.14159265358f;
const float g_GoldenRatio = 1.61803398875f;

vec2 GetBlueNoise2D(ivec2 pixel, int frame, int sampleIndex)
{
	ivec2 offset = ivec2(
		frame * 59 + sampleIndex * 73,
		frame * 157 + sampleIndex * 197
	);
	ivec2 noiseCoord = (pixel + offset) & 511;
	vec4 noise = texelFetch(u_BlueNoiseTexture, noiseCoord, 0);
	return noise.gb;
}

vec3 RandomDirectionOnHemisphere(vec3 normal, ivec2 pixel, int frame, int sampleIndex)
{
	vec2 n = GetBlueNoise2D(pixel, frame, sampleIndex);
	float r0 = n.x, r1 = n.y;

	float temporalOffset = float(frame) * g_GoldenRatio;
	float sampleOffset = float(sampleIndex) * 0.618034;
	r0 = fract(r0 + temporalOffset + sampleOffset);

	float cosTheta = sqrt(1.0 - r1);
	float sinTheta = sqrt(r1);
	float phi = 2.0 * g_PI * r0;

	float x = sinTheta * cos(phi);
	float y = sinTheta * sin(phi);
	float z = cosTheta;

	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, normal));
	vec3 bitangent = cross(normal, tangent);

	return normalize(x * tangent + y * bitangent + z * normal);
}

bool IsVoxelOccluded(ivec3 cell, int mip_level)
{
	ivec3 block_pos = cell >> 1;
	ivec3 local_pos = cell & 1;

	int bit_index = local_pos.x + local_pos.y * 2 + local_pos.z * 4;
	uint vpacked = texelFetch(u_ShadowMap, block_pos, mip_level).r;

	return ((vpacked >> bit_index) & 1u) != 0u;
}

bool RaycastShadowMapVariableFidelity(vec3 origin, vec3 direction, out float t, const int maxDistanceMeters, const int mipLevel)
{
	const float PackFactor = 2.0f;

	const float voxelScale = g_ShadowLODScales[mipLevel];

	// Blocks
	ivec3 mapDimensions = textureSize(u_ShadowMap, mipLevel);

	vec3 worldspaceExtents = (mapDimensions * PackFactor * voxelScale) / 2.0f;
	vec3 boundsMin = -worldspaceExtents;

	vec3 voxelsPerUnit = vec3(1.0f / voxelScale);
	vec3 entry = (origin - boundsMin) * voxelsPerUnit;

	// early exit
	if (any(lessThan(entry, vec3(0))) || any(greaterThanEqual(entry, vec3(mapDimensions * PackFactor)))) {
		t = float(maxDistanceMeters);
		return false;
	}

	vec3 step = sign(direction);
	vec3 delta = abs(1.0f / direction);
	ivec3 pos = ivec3(clamp(floor(entry), vec3(0.0f), vec3(mapDimensions * PackFactor - 1)));
	vec3 tMax = (vec3(pos) - entry + max(step, 0.0)) / direction;

	int maxSteps = int(maxDistanceMeters / voxelScale);
	ivec3 iStep = ivec3(step);

	for (int i = 0; i < maxSteps; i++) {
		if (IsVoxelOccluded(pos, 0)) {
			// Find which axis we just crossed
			int axis = (tMax.x < tMax.y) ? ((tMax.x < tMax.z) ? 0 : 2) : ((tMax.y < tMax.z) ? 1 : 2);
			t = (tMax[axis] - delta[axis]) * voxelScale;
			return true;
		}

		// branchless min select
		bvec3 mask = lessThan(tMax.xyz, tMax.yzx);
		mask = mask && lessThanEqual(tMax.xyz, tMax.zxy);

		pos += ivec3(mask) * iStep;
		tMax += vec3(mask) * delta;

		// exit map?? :(
		if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, mapDimensions * PackFactor))) {
			break;
		}
	}

	t = float(maxDistanceMeters);
	return false;
}

float CastAmbientOcclusionRay(vec3 origin, vec3 direction)
{
	vec3 rayOrigin = origin;
	float totalTraveled = 0.0f;
	float t;

	const int LOD0Dist = 1;

	// LOD 0
	rayOrigin = origin + direction * g_ShadowLODScales[0];
	bool hit = RaycastShadowMapVariableFidelity(rayOrigin, direction, t, LOD0Dist, 0);
	return clamp(t / LOD0Dist, 0.0f, 1.0f);

	//for (int lod = 0; lod < 3; lod++) {
	//	rayOrigin = origin + direction * ((totalTraveled + float(lod == 0)) * 1.01f * g_ShadowLODScales[lod] * 2.0f); // brless first mip offset
	//
	//	bool hit = RaycastShadowMapVariableFidelity(rayOrigin, direction, t, g_AORayDistances[lod], lod);
	//	totalTraveled += t;
	//
	//	if (hit) {
	//		return clamp(totalTraveled / g_AORayTotalDistance, 0.0, 1.0);
	//	}
	//} 
	//
	//return 1.0f; // no hit br
}

vec2 GetLastFrameUVFromThisFrameWorldPos(vec3 worldPos)
{
	vec4 prevClip = u_PrevFrameViewProjection * vec4(worldPos, 1.0f);
	if (prevClip.w <= 0.0f)
		return vec2(-1.0f);
	vec2 prevNdc = prevClip.xy / prevClip.w;
	return prevNdc * 0.5f + 0.5f; // -1,1 -> 0, 1
}

vec3 ReconstructWorldSpaceFromDepth(vec2 uv)
{
	float depth = texture(u_Depth, uv).r;

	float z = depth * 2.0f - 1.0f;
	vec4 ndcPosition = vec4(uv * 2.0f - 1.0f, z, 1.0f);  // x, y in [-1, 1], z in [-1, 1]
	vec4 cameraSpacePosition = u_InverseProjection * ndcPosition;
	cameraSpacePosition /= cameraSpacePosition.w;

	vec4 worldPos = u_InverseView * cameraSpacePosition;
	return worldPos.xyz / worldPos.w;
}

float LinearizeDepth(float depth)
{
	const float near = 0.1f, far = 500.0f;
	float z = 1.0f - depth;
	return (far * near) / (far - z * (far - near));
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
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	vec2 viewport = vec2(imageSize(u_Output));
	vec2 uv = (vec2(pixel) + 0.5f) / viewport;

	float depth = texture(u_Depth, uv).r;
	if (depth == 0.0f)
	{
		imageStore(u_Output, pixel, vec4(0.0f));
		return;
	}
	vec3 worldSpaceFragment = ReconstructWorldSpaceFromDepth(uv);

	const float ao_cutoff_distance = 110.0f;
	vec3 planar_cam = vec3(u_CameraPos.x, 0.0f, u_CameraPos.z);
	vec3 planar_worldspace = vec3(worldSpaceFragment.x, 0.0f, worldSpaceFragment.z);
	float cameraDistance = distance(planar_worldspace, planar_cam);
	if (cameraDistance > ao_cutoff_distance) // cut dat out br
	{
		imageStore(u_Output, pixel, vec4(0.0f));
		return;
	}

	vec3 normal = texture(u_Normal, uv).xyz;

	float this_frame_ao = 0.0f;
	const int AmbientOcclusionRaysPerPixel = 2;
	for (int i = 0; i < AmbientOcclusionRaysPerPixel; i++)
	{
		vec3 direction = RandomDirectionOnHemisphere(normal, pixel, u_FrameNumber, i);
		float norm = CastAmbientOcclusionRay(worldSpaceFragment, direction);

		this_frame_ao += norm;
	}
	this_frame_ao /= AmbientOcclusionRaysPerPixel;

	vec2 previousUV = GetLastFrameUVFromThisFrameWorldPos(worldSpaceFragment);
	bool inBounds = all(greaterThan(previousUV, vec2(0.0f))) && all(lessThan(previousUV, vec2(1.0f)));
	if (!inBounds)
	{
		imageStore(u_Output, pixel, vec4(this_frame_ao));
		return;
	}

	vec2 pixelSize = 1.0f / viewport;

	// we was just in da neighborhood
	float aoPrev = texture(u_AORead, previousUV).r;
	float ao1 = texture(u_AORead, previousUV + vec2(-pixelSize.x, 0.0f)).r;
	float ao2 = texture(u_AORead, previousUV + vec2(pixelSize.x, 0.0f)).r;
	float ao3 = texture(u_AORead, previousUV + vec2(0.0f, -pixelSize.y)).r;
	float ao4 = texture(u_AORead, previousUV + vec2(0.0f, pixelSize.y)).r;
	
	float mi = min(min(min(min(aoPrev, ao1), ao2), ao3), ao4);
	float ma = max(max(max(max(aoPrev, ao1), ao2), ao3), ao4);
	
	// Clamp current frame to previous frame's neighborhood
	float this_ao = clamp(this_frame_ao, mi, ma);
	//float this_ao = this_frame_ao;

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
	float velocityAlpha = clamp(motionLen * 50.0f, 0.0f, 1.0f);

	float historyConfidence = depthWeight * normalWeight;

	// blend factor
	const float baseAlpha = 0.05f, maxAlpha = 0.95f;
	float alpha = mix(maxAlpha, baseAlpha, clamp(historyConfidence, 0.0f, 1.0f));

	// motion discards history
	alpha = max(alpha, velocityAlpha);

	float result_ao_scalar = mix(aoPrev, this_frame_ao, alpha);
	imageStore(u_Output, pixel, vec4(result_ao_scalar));
}