#include "pch.h"

#include "VoxelMesh.h"
#include "Terrain.h"

#include "rendering/Texture.h"
#include "rendering/scene/SceneRenderer.h"

#include <simplex_noise/SimplexNoise.h>

namespace Engine {

	VoxelMesh Terrain::generate_terrain(const GenerationParameters& params)
	{
		uint8_t* voxel_data = new uint8_t[params.width * params.height * params.width]{};

		SimplexNoise noise
		{ 
			params.noise.frequency,
			params.noise.amplitude,
			params.noise.lacunarity,
			params.noise.persistence
		};

		uint32_t palette[256]{};
		palette[0] = encode_rgba(0, 0, 0, 0);
		for (int i = 1; i < 16; i++)
		{
			palette[i] = encode_rgba(Color(i, i, i, 16.0f) / 16.0f);
		}

		uint32_t w = params.width;
		uint32_t h = params.height;

		for (uint32_t x = 0; x < w; x++)
		{
			for (uint32_t z = 0; z < w; z++)
			{
				float fx = (float)x * params.noise.scale;
				float fz = (float)z * params.noise.scale;

				float value = (noise.fractal(params.noise.octaves, fx, fz) + 1.0f) / 2.0f; // 0, 1
				uint32_t y = glm::floor(value * (h - 1));

				size_t voxel_index = flatten_index_3d({ x, y, z }, { w, h, w });
				voxel_data[voxel_index] = y + 1;

				if (y > 0)
					for (uint32_t y1 = y - 1; y1 > 0; y1--)
					{
						size_t under_index = flatten_index_3d({ x, y1, z }, { w, h, w });
						voxel_data[under_index] = y1;
					}
			}
		}

		VoxelMesh mesh = VoxelMesh::build_from_voxels(voxel_data, w, h, w, palette, 2);

		// DEBUG: direct update shadow map
		auto& shadowMap = SceneRenderer::s_ShadowMap;

		if (!shadowMap)
		{
			shadowMap = Texture3D::create(w, h, w, TextureFormat::R8UI, 3);
		}

		shadowMap->set_data(voxel_data);
		shadowMap->generate_mips();

		delete[] voxel_data;

		return mesh;
	}

}