#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;

void main()
{
	gl_Position = vec4(a_Position, 0.0f, 1.0f);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout(binding = 0) uniform sampler2D u_Albedo;
layout(binding = 1) uniform sampler2D u_Normals;
layout(binding = 2) uniform sampler2D u_ComputeAO;
layout(binding = 3) uniform sampler2D u_Lighting;
layout(binding = 4) uniform sampler2D u_Depth;

uniform vec2 u_ViewportDims;

uniform mat4 u_InverseView;
uniform mat4 u_InverseProjection;
uniform vec3 u_CameraPos;

uniform int u_Output;

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

float GetAOFadeFromPixelPosition(vec3 worldPos)
{
	const float min_dist = 80.0f;
	const float range = 30.0f;

	vec3 planarCam = u_CameraPos;
	planarCam.y = 0.0f;
	worldPos.y = 0.0f;

	float dist = distance(planarCam, worldPos);

	if (dist < min_dist)
		return 1.0f;

	float clamped = clamp(dist, min_dist, min_dist + range);
	float t = (clamped - min_dist) / range;
	return mix(1.0f, 0.0f, t);
}

void main()
{
	vec2 uv = gl_FragCoord.xy / u_ViewportDims;

	vec4 albedo = texture(u_Albedo, uv);
	float ao = texture(u_ComputeAO, uv).r;
	vec3 normal = texture(u_Normals, uv).xyz;
	vec4 lighting = texture(u_Lighting, uv);
	float depth = texture(u_Depth, uv).r;

	const float ambient_contribution = 0.5f;
	vec3 worldSpaceFragment = ReconstructWorldSpaceFromDepth(uv);
	float ao_dist_fade = GetAOFadeFromPixelPosition(worldSpaceFragment);

	vec4 final = mix(albedo, albedo * vec4(vec3(ao), 1.0f), mix(0.0f, ambient_contribution, ao_dist_fade));
	//vec4 final = mix(albedo, albedo * vec4(vec3(ao * ao_dist_fade), 1.0f), ambient_contribution);

	switch (u_Output)
	{
	case 0: o_Color = final; break;
	case 1: o_Color = vec4(ao, 0.0f, 0.0f, 1.0f); break;
	case 2: o_Color = vec4(worldSpaceFragment, 1.0f); break;
	case 3: o_Color = vec4(normal, 1.0f); break;
	case 4: o_Color = vec4(depth, 0.0f, 0.0f, 1.0f); break;
	}
}