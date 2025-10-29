#version 450 core

#extension GL_ARB_bindless_texture : enable

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(binding = 0, r8ui) uniform readonly uimage3D u_ReadMip;
layout(binding = 1, r8ui) uniform writeonly uimage3D u_WriteMip;

void main()
{
	ivec3 texel = ivec3(gl_GlobalInvocationID.xyz);
	ivec3 write_dims = imageSize(u_WriteMip);

	ivec3 read_dims = imageSize(u_ReadMip);
	ivec3 read_base = texel * 2;

	uint has_voxels = 0u;
	for (int x = 0; x < 2; x++)
	for (int y = 0; y < 2; y++)
	for (int z = 0; z < 2; z++)
	{
		ivec3 read_pos = read_base + ivec3(x, y, z);

		uint data = imageLoad(u_ReadMip, read_pos).r;
		if (data != 0u)
			has_voxels = 1u;
	}

	imageStore(u_WriteMip, texel, uvec4(has_voxels == 0u ? 0 : 125));
}