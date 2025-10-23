#version 450 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(binding = 0, r8ui) uniform writeonly uimage3D u_ChunkTexture;
layout(binding = 1, r8ui) uniform writeonly uimage3D u_ShadowMap;

layout(std430, binding = 0) readonly buffer HeightBuffer
{
	float HeightMap[];
};

uniform float u_VoxelScale = 0.1f;
uniform ivec3 u_ChunkDimensions;

void main()
{
	ivec3 voxel = ivec3(gl_GlobalInvocationID.xyz);

	if (any(greaterThanEqual(voxel, u_ChunkDimensions)))
		return;

	int index = voxel.z * u_ChunkDimensions.x + voxel.x;
	float height_sample = HeightMap[index];
	int voxel_height = int(height_sample * u_ChunkDimensions.y);

	// 64x16x64
	// 0..15
	int height_index = int(floor(height_sample * 15));

	if (voxel.y < voxel_height) {
		imageStore(u_ChunkTexture, voxel, uvec4(height_index));
		imageStore(u_ShadowMap, voxel, uvec4(1));
	}
}