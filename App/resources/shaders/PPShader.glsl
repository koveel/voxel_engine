#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;

void main()
{
	gl_Position = vec4(a_Position, 0.0f, 1.0f);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

uniform vec2 u_ViewportDims;

in vec2 o_UV;

#include "raytracing.glinc"
layout(binding = 1) uniform sampler2D u_Albedo;
layout(binding = 2) uniform sampler2D u_Normal;
layout(binding = 3) uniform sampler2D u_Lighting;

layout(binding = 4) uniform sampler3D u_ShadowMap;
layout(binding = 5) uniform sampler2D u_BlueNoiseTexture;

uniform int u_FrameNumber;
const float GoldenRatio = 1.61803398875f;

float GetBlueNoise(vec2 uvseed)
{
	vec2 coord = uvseed / vec2(512, 512);
	float sampl = texture(u_BlueNoiseTexture, coord).a;
	return mod(sampl + GoldenRatio * (u_FrameNumber % 100), 1.0f);
}

const float M_PI = 3.14159265358f;
vec3 RandomDirectionOnHemisphere(vec2 uv, vec3 normal)
{
	float r0 = GetBlueNoise(uv);
	float r1 = GetBlueNoise(uv * r0);

	float theta = 2.0f * M_PI * r0;
	float phi = acos(1.0f - 2.0f * r1);
	float x = sin(phi * cos(theta));
	float y = sin(phi * sin(theta));
	float z = cos(phi);

	vec3 point = normalize(vec3(x, y, z));
	vec3 ps = sign(point);
	vec3 ns = sign(normal);

	// Make direction match normal direction
	point.x *= ps.x != ns.x ? -1.0f : 1.0f;
	point.y *= ps.y != ns.y ? -1.0f : 1.0f;
	point.z *= ps.z != ns.z ? -1.0f : 1.0f;

	return point;
}

const float VoxelScale = 0.1f;
const vec3 ShadowMapDimensions = ivec3(250);

const int AmbientOcclusionRaysPerPixel = 2;
const int AmbientOcclusionRaysMaxDistanceMeters = 32;

bool RaycastShadowMap(vec3 origin, vec3 direction, out float t)
{
	// Bounds (shadow map always centered around origin)
	vec3 worldspaceExtents = (ShadowMapDimensions * VoxelScale) / 2.0f;
	vec3 p0 = -worldspaceExtents, p1 = worldspaceExtents;

	const vec3 voxelsPerUnit = 1.0f / vec3(VoxelScale);

	origin += direction * VoxelScale;
	vec3 entry = ((origin) - p0) * voxelsPerUnit;

	vec3 step = sign(direction);
	vec3 delta = abs(1.0f / direction);

	vec3 pos = clamp(floor(entry), vec3(0.0f, 0.0f, 0.0f), ShadowMapDimensions);
	vec3 tMax = (pos - entry + max(step, 0)) / direction;

	int axis = 0; // x, y, z
	for (int i = 0; i < AmbientOcclusionRaysMaxDistanceMeters * 10; i++)
	{
		ivec3 voxelPos = ivec3(pos);
		float sampl = texelFetch(u_ShadowMap, voxelPos, 0).r;
		if (sampl != 0.0f) // hit something?
		{
			t = (tMax[axis] - delta[axis]) / voxelsPerUnit[axis];
			return true;
		}

		if (tMax.x < tMax.y)
		{
			if (tMax.x < tMax.z)
			{
				pos.x += step.x;
				if (pos.x < 0 || pos.x >= ShadowMapDimensions.x)
					break;
				t = tMax.x;
				tMax.x += delta.x;
				axis = 0;
			}
			else
			{
				pos.z += step.z;
				if (pos.z < 0 || pos.z >= ShadowMapDimensions.z)
					break;
				t = tMax.z;
				tMax.z += delta.z;
				axis = 2;
			}
		}
		else
		{
			if (tMax.y < tMax.z)
			{
				pos.y += step.y;
				if (pos.y < 0 || pos.y >= ShadowMapDimensions.y)
					break;
				t = tMax.y;
				tMax.y += delta.y;
				axis = 1;
			}
			else
			{
				pos.z += step.z;
				if (pos.z < 0 || pos.z >= ShadowMapDimensions.z)
					break;
				t = tMax.z;
				tMax.z += delta.z;
				axis = 2;
			}
		}
	}

	// No hit
	t = AmbientOcclusionRaysMaxDistanceMeters;
	return false;
}

uniform int u_Output;
void main()
{
	vec2 uv = gl_FragCoord.xy / u_ViewportDims;

	float depth = texture(u_DepthTexture, uv).r;
	if (depth == 1.0f) return;

	vec4 albedo = texture(u_Albedo, uv);
	vec3 normal = texture(u_Normal, uv).xyz;
	vec3 worldSpaceFragment = ReconstructWorldSpaceFromDepth(uv);

	float average_t = 0.0f;
	for (int i = 0; i < AmbientOcclusionRaysPerPixel; i++)
	{
		vec3 origin = worldSpaceFragment;
		vec3 direction = RandomDirectionOnHemisphere(gl_FragCoord.xy * (i + 1), normal);

		float t;
		RaycastShadowMap(origin, direction, t);
		average_t += t;
	}
	average_t /= AmbientOcclusionRaysPerPixel;
	
	const float AmbientContribution = 1.0f;

	float linear_t = average_t / AmbientOcclusionRaysMaxDistanceMeters;
	vec4 ambient = vec4(vec3(1.0f - AmbientContribution), 1.0f);
	vec4 ao_time = vec4(linear_t, linear_t, linear_t, 0.0f) * AmbientContribution;

	vec4 result = albedo * (ambient + ao_time);
	result = 1.0f - exp(-result);

	switch (u_Output)
	{
	case 0: color = albedo; break;
	case 1: color = result; break;
	case 2: color = vec4(normal, 1.0f); break;
	case 3: color = vec4(depth, 0.0f, 0.0f, 1.0f); break;
	case 4: color = vec4(worldSpaceFragment, 1.0f); break;
	default:
		color = vec4(1.0f, 0.0f, 1.0f, 1.0f);
		break;
	}
}