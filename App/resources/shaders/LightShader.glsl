#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transformation;

out vec3 o_Vertex;

void main()
{
	vec4 transformed = u_Transformation * vec4(a_Position, 1.0f);
	o_Vertex = transformed.xyz;
	gl_Position = u_ViewProjection * transformed;
}

#type fragment
#version 450 core

layout(location = 6) out vec4 color;

in vec3 o_Vertex;

uniform vec3 u_Color;
uniform vec3 u_VolumeCenter;
uniform float u_LightRadius;
uniform float u_LightIntensity;

uniform vec2 u_ViewportDims;

#include "raytracing.glinc"

layout(binding = 2) uniform sampler2D u_Normals;

void main()
{
	vec2 uv = gl_FragCoord.xy / u_ViewportDims;
	vec3 normal = texture(u_Normals, uv).xyz;
	vec3 worldPos = ReconstructWorldSpaceFromDepth(uv);
	
	vec3 toLight = u_VolumeCenter - worldPos;
	float dist = length(toLight);
	vec3 directionToLight = normalize(toLight);

	float falloff = u_LightIntensity * (1.0 - smoothstep(0.0, u_LightRadius, dist));

	float NdotL = max(0.0, dot(normal, directionToLight));
	float intensity = falloff * NdotL;

	if (intensity < 0.01) {
		discard;
		return;
	}

	const int LOD = 0;
	const float voxel_scale = g_ShadowLODScales[LOD];
	vec3 rayOrigin = worldPos + directionToLight * (voxel_scale * 0.5f);
	float d2 = distance(rayOrigin, u_VolumeCenter);

	float t;
	int maxDistMeters = int(ceil(dist / voxel_scale)) + 1;

	bool occluded = RaycastShadowMapVariableFidelity(rayOrigin, directionToLight, t, maxDistMeters, LOD);

	const float bias = voxel_scale;
	if (occluded && t < dist) {
		discard;
		return;
	}

	color = vec4(u_Color * intensity, 1.0);
}