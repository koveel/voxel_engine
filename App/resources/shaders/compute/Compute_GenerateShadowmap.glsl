#version 450 core

#extension GL_ARB_bindless_texture : enable

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(binding = 0, r8ui) uniform writeonly uimage3D u_ShadowMap;
layout(bindless_image, r8ui) uniform readonly uimage3D u_ChunkHandles[9];

uniform ivec3 u_ChunkDimensions;
uniform vec3 u_CenterChunkPosition;

uniform int u_ChunkCount;

void main()
{
	ivec3 global_voxel = ivec3(gl_GlobalInvocationID.xyz);

	const ivec2 GridSize = ivec2(3, 3);
	ivec3 total_bounds = ivec3(
		u_ChunkDimensions.x * GridSize.x,
		u_ChunkDimensions.y,
		u_ChunkDimensions.z * GridSize.y
	);
	
	if (any(greaterThanEqual(global_voxel, total_bounds)))
		return;
	
	ivec2 chunk_index2D = ivec2(
		global_voxel.x / u_ChunkDimensions.x,
		global_voxel.z / u_ChunkDimensions.z
	);
	
	int chunk_index = chunk_index2D.y * GridSize.x + chunk_index2D.x;
	ivec3 local_voxel = ivec3(
		global_voxel.x % u_ChunkDimensions.x,
		global_voxel.y,
		global_voxel.z % u_ChunkDimensions.z
	);
	
	uint voxel = imageLoad(u_ChunkHandles[chunk_index], local_voxel).r;
	uint v = uint(voxel != 0);

	imageStore(u_ShadowMap, global_voxel, uvec4(v));
}