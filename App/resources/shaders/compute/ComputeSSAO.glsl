#version 450 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

//layout(binding = 1) uniform sampler2D u_BlueNoiseTexture;

layout(binding = 0) uniform sampler2D u_Depth;
layout(binding = 1) uniform sampler2D u_Normal;
layout(binding = 2) uniform sampler2D u_AORead;

layout(r8, binding = 0) uniform writeonly image2D u_Output;

uniform mat4 u_InverseView;
uniform mat4 u_ViewProjection;
uniform mat4 u_InverseProjection;

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
	const float near = 0.1f, far = 100.0f;
	float z = depth * 2.0 - 1.0;
	return (2.0 * near * far) / (far + near - z * (far - near));
}

vec3 ReconstructViewPos(vec2 uv, float depth)
{
	float z = depth * 2.0f - 1.0f;
	vec4 clip = vec4(uv * 2.0f - 1.0f, z, 1.0f);
	vec4 view = u_InverseProjection * clip;
	return view.xyz / view.w;
}

vec3 ReconstructPosition(vec2 uv, float depth)
{
	vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
	vec4 view = u_InverseProjection * clip;
	return view.xyz / view.w;
}

// (d, nx, ny, nz)
vec4 SampleParameters(vec2 uv)
{
	float depth = LinearizeDepth(texture(u_Depth, uv).r);
	vec3 normal = texture(u_Normal, uv).xyz;

	return vec4(depth, normal);
}

void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	vec2 viewport = vec2(imageSize(u_Output));
	vec2 uv = (vec2(pixel) + 0.5f) / viewport;

	float depth = texture(u_Depth, uv).r;
	if (depth == 1.0f)
	{
		imageStore(u_Output, pixel, vec4(0.0f));
		return;
	}
	vec3 normal = texture(u_Normal, uv).xyz;
	//depth = LinearizeDepth(depth);

	vec3 pos = ReconstructPosition(uv, depth);
	const vec3 kernel[8] = vec3[](
		vec3(1, 0, 0), vec3(-1, 0, 0),
		vec3(0, 1, 0), vec3(0, -1, 0),
		vec3(0, 0, 1), vec3(0, 0, -1),
		normalize(vec3(1, 1, 1)), normalize(vec3(-1, -1, 1))
	);

	const float u_Radius = 0.5f;
	const float u_Bias = 0.025f;
	float occlusion = 0.0;
	for (int i = 0; i < 8; i++)
	{
		// Project a sample in tangent space
		vec3 samplePos = pos + kernel[i] * u_Radius;

		// Convert to clip-space to sample depth buffer
		vec4 offsetClip = inverse(u_InverseProjection) * vec4(samplePos, 1.0);
		offsetClip.xyz /= offsetClip.w;
		vec2 offsetUV = offsetClip.xy * 0.5 + 0.5;

		// Skip invalid coords
		if (any(lessThan(offsetUV, vec2(0.0))) || any(greaterThan(offsetUV, vec2(1.0))))
			continue;

		float sampleDepth = texture(u_Depth, offsetUV).r;
		if (sampleDepth == 1.0) continue;

		// Compare depth difference
		float sampleViewZ = LinearizeDepth(sampleDepth);
		float range = u_Radius / abs(pos.z - sampleViewZ + 1e-4);
		float weight = smoothstep(0.0, 1.0, range);

		// If sample is behind current point, it occludes
		if (sampleViewZ > pos.z + u_Bias)
			occlusion += weight;
	}

	occlusion = 1.0 - (occlusion / 8.0);
	imageStore(u_Output, pixel, vec4(vec3(occlusion), 1.0));
}