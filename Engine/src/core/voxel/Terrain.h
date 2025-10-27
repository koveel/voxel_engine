#pragma once

#include "rendering/Texture.h"
#include "rendering/Buffer.h"

#include <functional>
#include <map>

namespace Engine {
	
	class ComputeShader;
	class ShaderStorageBuffer;

	struct TerrainChunkIndex
	{
		Int2 index{};
		Int2 origin_chunk{}; // cam/player chunk
		
		// distance
		bool operator<(const TerrainChunkIndex& other)
		{
			Int2 d0 = index - origin_chunk;
			Int2 d1 = other.index - origin_chunk;
			int len0 = d0.x * d0.x + d0.y * d0.y;
			int len1 = d1.x * d1.x + d1.y * d1.y;

			return len0 < len1;
		}
	};

	struct TerrainChunk
	{
		static constexpr size_t Width = 256, Height = 64;

		VoxelMesh mesh;
		BindlessTexture3D bindless_image;
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

		owning_ptr<ComputeShader> m_ShadowMapMipGenerationShader;
		owning_ptr<ComputeShader> m_ShadowMapBaseMipGenerationShader;

		//std::unordered_map<Int2, TerrainChunk> m_ChunkTable;
		std::map<Int2, TerrainChunk> m_ChunkTable;

		owning_ptr<Texture3D> m_ShadowMap;
	};

} 