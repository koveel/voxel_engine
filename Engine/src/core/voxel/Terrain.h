#pragma once

#include "rendering/Buffer.h"

namespace Engine {
	
	class ComputeShader;
	class ShaderStorageBuffer;

	struct TerrainGenerationParameters
	{
		struct
		{
			float amplitude = 0.8f;
			float frequency = 0.8f;
			float lacunarity = 0.6f;
			float persistence = 0.8f;
			uint32_t octaves = 3;
			float scale = 1.0f / 30.0f;
		} noise;

		uint32_t width = 0; // x and z
		uint32_t height = 0;
	};

	class TerrainGenerator
	{
	public:
		TerrainGenerator();

		VoxelMesh generate_chunk();
		VoxelMesh generate_terrain(const TerrainGenerationParameters& params);
	public:
		owning_ptr<ShaderStorageBuffer> m_HeightMapSSBO;
		owning_ptr<ComputeShader> m_ChunkGenerationShader;
	};

} 