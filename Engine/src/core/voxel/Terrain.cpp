#include "pch.h"

#include "VoxelMesh.h"
#include "Terrain.h"

#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "rendering/scene/SceneRenderer.h"

#include <simplex_noise/SimplexNoise.h>

namespace Engine {

	static constexpr size_t ChunkWidth = 128, ChunkHeight = 16;

	TerrainGenerator::TerrainGenerator()
	{
		m_HeightMapSSBO = ShaderStorageBuffer::create(nullptr, ChunkWidth * ChunkWidth * sizeof(float));
		m_ChunkGenerationShader = ComputeShader::create("resources/shaders/compute/Compute_GenerateTerrain.glsl");
	}

	VoxelMesh TerrainGenerator::generate_chunk()
	{
		if (false)
		{
			static TerrainGenerationParameters parameters = {
				.noise = {
					.amplitude = 2.0f,
					.frequency = 0.5f,
					.lacunarity = 2.0f,
					.persistence = 0.7f,
					.octaves = 4,
				},
				.width = ChunkWidth,
				.height = ChunkHeight,
			};

			// Height map
			SimplexNoise noise{
				parameters.noise.frequency,
				parameters.noise.amplitude,
				parameters.noise.lacunarity,
				parameters.noise.persistence
			};

			static float* height_map = new float[ChunkWidth * ChunkWidth] {};

			for (uint32_t x = 0; x < ChunkWidth; x++)
			{
				for (uint32_t y = 0; y < ChunkWidth; y++)
				{
					float fx = (float)x * parameters.noise.scale;
					float fz = (float)y * parameters.noise.scale;
					float height = (noise.fractal(parameters.noise.octaves, fx, fz) + 1.0f) / 2.0f; // 0, 1

					size_t index = flatten_index_2d({ x, y }, { ChunkWidth, ChunkWidth });
					height_map[index] = height;
				}
			}

			m_HeightMapSSBO->bind(0);
			m_HeightMapSSBO->set_data(height_map, ChunkWidth * ChunkWidth * sizeof(float));
		}

		// Generate chunk
		VoxelMesh chunk_mesh;
		auto texture = Texture3D::create(ChunkWidth, ChunkHeight, ChunkWidth, TextureFormat::R8UI, 1);

		// direct create and update shadow map (for now)
		auto& shadowMap = SceneRenderer::s_ShadowMap;
		if (!shadowMap)
			shadowMap = Texture3D::create(ChunkWidth, ChunkHeight, ChunkWidth, TextureFormat::R8UI, 3);

		texture->bind_as_image(0, TextureAccessMode::Write, 0);
		shadowMap->bind_as_image(1, TextureAccessMode::Write, 0);

		m_ChunkGenerationShader->set_int3("u_ChunkDimensions", texture->get_dimensions());

		constexpr uint32_t LocalSizeInShader = 4;
		m_ChunkGenerationShader->dispatch(ChunkWidth / LocalSizeInShader, ChunkHeight / LocalSizeInShader, ChunkWidth / LocalSizeInShader);

		// Generate mips...

		chunk_mesh.m_Texture = std::move(texture);
		return chunk_mesh;
	}

	void TerrainGenerator::regenerate_chunk(VoxelMesh& mesh, Float3 noiseOffset)
	{
		auto& texture = mesh.m_Texture;
		auto& shadowMap = SceneRenderer::s_ShadowMap;

		uint8_t v[ChunkWidth * ChunkHeight * ChunkWidth]{};
		texture->set_data(v);
		shadowMap->set_data(v);

		texture->bind_as_image(0, TextureAccessMode::Write, 0);
		shadowMap->bind_as_image(1, TextureAccessMode::Write, 0);

		m_ChunkGenerationShader->set_float3("u_NoiseOffset", noiseOffset);
		m_ChunkGenerationShader->set_int3("u_ChunkDimensions", texture->get_dimensions());

		constexpr uint32_t LocalSizeInShader = 4;
		m_ChunkGenerationShader->dispatch(ChunkWidth / LocalSizeInShader, ChunkHeight / LocalSizeInShader, ChunkWidth / LocalSizeInShader);

		// Generate mips...
	}

	VoxelMesh TerrainGenerator::generate_terrain(const TerrainGenerationParameters& params)
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