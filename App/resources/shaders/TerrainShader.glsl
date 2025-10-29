#type vertex
#version 450 core
#extension GL_ARB_bindless_texture : enable
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;
out vec3 v_VertexWorldSpace;
out flat int v_Instance;

struct ChunkInstance
{
	mat4 transformation;
	uint64_t voxel_texture_handle;
};

layout(std430, binding = 0) readonly buffer InstanceData
{
	ChunkInstance chunks[];
};

void main()
{
	ChunkInstance instance = chunks[gl_InstanceID];

	vec4 transformed = instance.transformation * vec4(a_Position, 1.0f);
	v_VertexWorldSpace = transformed.xyz;
	v_Instance = gl_InstanceID;

	gl_Position = u_ViewProjection * transformed;
}

#type fragment
#version 450 core

#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out vec3 o_Normal;

in vec3 v_VertexWorldSpace;
in flat int v_Instance;

layout(binding = 1) uniform usampler3D u_ShadowMap;
layout(binding = 2) uniform sampler2D u_DepthOcclusion;

layout(binding = 3) uniform sampler2D u_MaterialPalette;

struct ChunkInstance
{
	mat4 transformation;
	uint64_t voxel_texture_handle;
};

layout(std430, binding = 0) readonly buffer InstanceData
{
	ChunkInstance chunks[];
};

uniform mat4 u_ViewProjection;
uniform vec3 u_CameraPosition;
uniform vec2 u_ViewportDims;

uniform int u_MaterialIndex;
uniform int u_MipLevel = 0;

float RayAABB_fast(vec3 origin, vec3 invDir, vec3 p0, vec3 p1, ivec3 sign)
{
	vec3 bounds0 = mix(p0, p1, vec3(sign));
	vec3 bounds1 = mix(p1, p0, vec3(sign));

	vec3 tmin = (bounds0 - origin) * invDir;
	vec3 tmax = (bounds1 - origin) * invDir;

	// intersection guaranteed
	return max(max(tmin.x, tmin.y), tmin.z);
}

float RayAABB(vec3 ro, vec3 invrd, vec3 p0, vec3 p1)
{
	float tmin = 0, tmax = 1e30f;

	for (int axis = 0; axis < 3; axis++)
	{
		float t1 = (p0[axis] - ro[axis]) * invrd[axis];
		float t2 = (p1[axis] - ro[axis]) * invrd[axis];

		float dmin = min(t1, t2);
		float dmax = max(t1, t2);

		tmin = max(dmin, tmin);
		tmax = min(dmax, tmax);
	}

	return tmin;
}

bool Approx(float a, float b)
{
	const float epsilon = 0.00001f;
	return abs(a - b) < epsilon;
}

void RaymarchVoxelMesh(
	vec3 rayOrigin, vec3 rayDirection, vec3 obbCenter,
	out int color, out vec3 normal, out float t, out ivec3 voxel,
	usampler3D voxel_texture
)
{
	const float BaseVoxelScale = 0.1f;
	float voxelScale = BaseVoxelScale * exp2(u_MipLevel);

	// Bounding box
	ivec3 mipDimensions = textureSize(voxel_texture, u_MipLevel);
	vec3 worldspaceExtents = (vec3(mipDimensions) * voxelScale) * 0.5f;
	vec3 p0 = obbCenter - worldspaceExtents;
	vec3 p1 = obbCenter + worldspaceExtents;

	ivec3 step = ivec3(sign(rayDirection));
	vec3 invDirection = 1.0f / rayDirection;

	float hit = RayAABB(rayOrigin, invDirection, p0, p1);
	//hit = mix(0.0f, hit, sign(hit));

	//hit = 0.0f;
	vec3 voxelsPerUnit = mipDimensions / (p1 - p0);
	vec3 entry = ((rayOrigin + rayDirection * (hit + 0.0001f)) - p0) * voxelsPerUnit;
	vec3 entryWorldspace = rayOrigin + rayDirection * hit;

	// DEPTH EARLY EXIT
	//vec4 entryClip = u_ViewProjection * vec4(entryWorldspace, 1.0);
	//
	//float ndcZ = entryClip.z / entryClip.w;  // -1..1
	//float windowDepth = ndcZ * 0.5 + 0.5;
	//
	//ivec2 texel = ivec2(gl_FragCoord.xy);
	//// 1 = near, 0 = far
	//float cur_depth = texelFetch(u_DepthOcclusion, texel, 0).r;
	//
	//// if ray starts further away, occluded and can skip
	//if (windowDepth < cur_depth) {
	//	discard;
	//	return;
	//}

	vec3 delta = abs(invDirection);
	ivec3 pos = ivec3(clamp(floor(entry), vec3(0.0f), vec3(mipDimensions - 1)));
	vec3 tMax = (vec3(pos) - entry + max(vec3(step), 0.0)) / rayDirection;

	int axis = 0;
	int maxSteps = mipDimensions.x + mipDimensions.y + mipDimensions.z;

	for (int i = 0; i < maxSteps; i++)
	{
		uint col = texelFetch(voxel_texture, pos, u_MipLevel).r;
		if (col != 0)
		{
			color = int(col);
			voxel = pos;

			// edge voxel
			if (i == 0)
			{
				t = hit;
				// Determine normal
				for (int a = 0; a < 3; a++)
				{
					float v = entryWorldspace[a];
					if (Approx(v, p0[a]) || Approx(v, p1[a]))
					{
						normal = vec3(0.0f);
						normal[a] = -step[a];
						break;
					}
				}

				return;
			}

			normal = vec3(0.0f);
			normal[axis] = -float(step[axis]);
			t = hit + (tMax[axis] - delta[axis]) / voxelsPerUnit[axis];

			return;
		}

		// branchless step
		bvec3 isMin = lessThan(tMax.xyz, tMax.yzx);
		isMin = isMin && lessThanEqual(tMax.xyz, tMax.zxy);

		pos += ivec3(isMin) * ivec3(step);
		tMax += vec3(isMin) * delta;
		axis = int(dot(vec3(isMin), vec3(0.0, 1.0, 2.0)));

		if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, mipDimensions)))
			break;
	}

	discard;
}

float LinearizeDepth(vec3 p)
{
	vec4 clipPos = u_ViewProjection * vec4(p, 1.0f);
	vec3 ndc = clipPos.xyz / clipPos.w;
	return (ndc.z + 1.0f) / 2.0f;
}

void main()
{
	vec3 cameraToPixel = normalize(v_VertexWorldSpace - u_CameraPosition);

	int colorIndex;
	vec3 normal;
	float t;
	ivec3 voxel;

	// march
	ChunkInstance chunk_instance = chunks[v_Instance];

	vec3 obb_center = vec3(chunk_instance.transformation[3]);
	usampler3D voxel_texture = usampler3D(chunk_instance.voxel_texture_handle);
	RaymarchVoxelMesh(u_CameraPosition, cameraToPixel, obb_center, colorIndex, normal, t, voxel, voxel_texture);

	// hitpoint / depth
	vec3 hitpoint = u_CameraPosition + cameraToPixel * t;
	float depth = LinearizeDepth(hitpoint);
	gl_FragDepth = depth;

	// normals
	o_Normal = normal;

	vec4 albedo = texelFetch(u_MaterialPalette, ivec2(colorIndex, u_MaterialIndex), 0);
	o_Albedo = albedo;
}