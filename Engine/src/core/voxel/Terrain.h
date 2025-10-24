#pragma once

#include "rendering/Buffer.h"

namespace Engine {
	
	class ComputeShader;
	class ShaderStorageBuffer;

	struct TerrainChunk
	{
		static constexpr size_t Width = 1024, Height = 16;

		VoxelMesh mesh;
		Int2 index{};
		Float3 position{};
	};

	class TerrainGenerator
	{
	public:
		TerrainGenerator();

		TerrainChunk generate_chunk(Int2 planar_chunk_index);
 		//void regenerate_chunk(VoxelMesh& mesh, Float3 noiseOffset);

		void regenerate_shadowmap(Int2 center_chunk);
	public:
		//owning_ptr<ShaderStorageBuffer> m_HeightMapSSBO;
		owning_ptr<ComputeShader> m_ChunkGenerationShader;

		//std::vector<VoxelMesh*> m_GeneratedChunks;
	};

} 