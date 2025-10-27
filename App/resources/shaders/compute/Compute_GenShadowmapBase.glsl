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
	const int PackFactor = 2;
	ivec3 block = ivec3(gl_GlobalInvocationID.xyz);

	const int GridSize = 3;
	ivec3 total_voxels = ivec3(
		u_ChunkDimensions.x * GridSize,
		u_ChunkDimensions.y,
		u_ChunkDimensions.z * GridSize
	);

	ivec3 total_blocks = ivec3(
		(total_voxels.x + PackFactor - 1) / PackFactor,
		(total_voxels.y + PackFactor - 1) / PackFactor,
		(total_voxels.z + PackFactor - 1) / PackFactor
	);

	// bounds test in block-space
	if (any(greaterThanEqual(block, total_blocks)))
		return;

	uint packed_data = 0u;
	for (int x = 0; x < 2; x++)
	for (int y = 0; y < 2; y++)
	for (int z = 0; z < 2; z++)
	{

		ivec3 voxel_pos = block * PackFactor + ivec3(x, y, z);
		if (any(lessThan(voxel_pos, ivec3(0))) || any(greaterThanEqual(voxel_pos, total_voxels)))
			continue;

		ivec2 chunk_index2D = ivec2(
			voxel_pos.x / u_ChunkDimensions.x,
			voxel_pos.z / u_ChunkDimensions.z
		);
		int chunk_index = chunk_index2D.y * GridSize + chunk_index2D.x;

		ivec3 local_voxel = ivec3(
			voxel_pos.x % u_ChunkDimensions.x,
			voxel_pos.y,
			voxel_pos.z % u_ChunkDimensions.z
		);
	
		uint voxel = imageLoad(u_ChunkHandles[chunk_index], local_voxel).r;
		if (voxel != 0u) {
			int	bit_index = x + (y << 1) + (z << 2);
			packed_data |= (1u << bit_index);
		}
	}
	
	imageStore(u_ShadowMap, block, uvec4(packed_data));
}