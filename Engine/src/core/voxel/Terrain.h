#pragma once

#include "rendering/Texture.h"
#include "rendering/Buffer.h"

namespace Engine {
	
	class ComputeShader;
	class ShaderStorageBuffer;

	struct TerrainChunk
	{
		static constexpr size_t Width = 1024, Height = 32;

		VoxelMesh mesh;
		BindlessTexture3D bindless_image_handle;
		Int2 index{};
		Float3 position{};
	};

	class TerrainGenerator
	{
	public:
		TerrainGenerator();

		TerrainChunk& generate_chunk(Int2 planar_chunk_index);

		void generate_shadowmap(Int2 center_chunk);
	public:
		owning_ptr<ComputeShader> m_ChunkGenerationShader;
		owning_ptr<ComputeShader> m_ShadowMapGenerationShader;

		std::unordered_map<Int2, TerrainChunk> m_ChunkTable;

		owning_ptr<Texture3D> m_ShadowMap;
	};

} 