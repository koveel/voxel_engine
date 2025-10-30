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
	int lod;
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

//layout(location = 7) out float o_Depth;

in vec3 v_VertexWorldSpace;
in flat int v_Instance;

struct ChunkInstance
{
	mat4 transformation;
	uint64_t voxel_texture_handle;
	int lod;
};

layout(std430, binding = 0) readonly buffer InstanceData
{
	ChunkInstance chunks[];
};

uniform mat4 u_ViewProjection;
uniform vec3 u_CameraPosition;

float RayAABB_fast(vec3 ro, vec3 invrd, vec3 p0, vec3 p1)
{
	vec3 sign = 1.0f - step(0.0f, invrd);

	vec3 bounds0 = mix(p0, p1, sign);
	vec3 bounds1 = mix(p1, p0, sign);

	vec3 tminV = (bounds0 - ro) * invrd;
	float tEnter = max(max(tminV.x, tminV.y), tminV.z);

	return max(tEnter, 0.0f);
}

bool RaymarchVoxelMesh(
	vec3 rayOrigin, vec3 rayDirection, vec3 obbCenter,
	usampler3D voxel_texture,
	out int color, out vec3 normal, out float t, out ivec3 voxel
)
{
	const int MipLevel = 2;

	const float BaseVoxelScale = 0.1f;
	float voxelScale = BaseVoxelScale * exp2(MipLevel);

	// Bounding box
	ivec3 MipLevelDimensions = textureSize(voxel_texture, MipLevel);
	vec3 worldspaceExtents = (vec3(MipLevelDimensions) * voxelScale) * 0.5f;
	vec3 p0 = obbCenter - worldspaceExtents;
	vec3 p1 = obbCenter + worldspaceExtents;

	ivec3 step = ivec3(sign(rayDirection));
	vec3 invDirection = 1.0f / rayDirection;

	float hit = RayAABB_fast(rayOrigin, invDirection, p0, p1);

	vec3 voxelsPerUnit = MipLevelDimensions / (p1 - p0);
	vec3 entry = ((rayOrigin + rayDirection * hit) - p0) * voxelsPerUnit;
	vec3 entryWorldspace = rayOrigin + rayDirection * hit;	

	vec3 delta = abs(invDirection);
	ivec3 pos = ivec3(clamp(floor(entry), vec3(0.0f), vec3(MipLevelDimensions - 1)));
	vec3 tMax = (vec3(pos) - entry + max(vec3(step), 0.0)) / rayDirection;

	int axis = 0;
	int maxSteps = MipLevelDimensions.x + MipLevelDimensions.y + MipLevelDimensions.z;

	for (int i = 0; i < maxSteps; i++)
	{
		uint col = texelFetch(voxel_texture, pos, MipLevel).r;
		if (col != 0)
		{
			color = int(col);
			voxel = pos;

			bool edgeVoxel = i != 0;
			t = mix(hit, hit + (tMax[axis] - delta[axis]) / voxelsPerUnit[axis], float(edgeVoxel));
			return true;
		}

		// branchless step
		bvec3 isMin = lessThan(tMax.xyz, tMax.yzx);
		isMin = isMin && lessThanEqual(tMax.xyz, tMax.zxy);

		pos += ivec3(isMin) * ivec3(step);
		tMax += vec3(isMin) * delta;
		axis = int(dot(vec3(isMin), vec3(0.0, 1.0, 2.0)));

		if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, MipLevelDimensions)))
			break;
	}

	return false;
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
	usampler3D voxel_texture = usampler3D(chunk_instance.voxel_texture_handle);
	vec3 chunk_center = vec3(chunk_instance.transformation[3]);

	bool hit = RaymarchVoxelMesh(u_CameraPosition, cameraToPixel, chunk_center, voxel_texture, colorIndex, normal, t, voxel);

	// hitpoint / depth
	vec3 hitpoint = u_CameraPosition + cameraToPixel * t;
	float depth = LinearizeDepth(hitpoint);

	if (hit)
		gl_FragDepth = depth;
	else
		discard;
}