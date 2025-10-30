#pragma once

#include "rendering/Texture.h"
#include "rendering/Buffer.h"

namespace Engine {
	
	class Shader;
	class ComputeShader;

	struct TerrainChunk
	{
		static constexpr size_t Width = 512, Height = 128;

		VoxelMesh mesh;
		BindlessTexture3D bindless_texture;
		Int2 index{};
		Float3 position{};

		uint8_t generated_lods = 0;
	};

	class TerrainGenerator
	{
	public:
		TerrainGenerator();

		// generates lowest LOD
		TerrainChunk& generate_chunk(Int2 chunk_index);
		TerrainChunk& generate_chunk_lod(Int2 chunk_index, uint32_t lod);

		void resort_chunks(Int2 origin);
		void generate_shadowmap(Int2 origin);

		void render_terrain(const Matrix4& viewProjection, Float3 camera);
	private:
		void generate_occlusion_mips_for_texture(Texture3D* texture, size_t textureMipCount);
		void dispatch_terrain_lod_gen_compute(TerrainChunk& chunk, uint32_t lod);

		void fill_instance_data(struct ChunkInstanceData* data, TerrainChunk& from, Int2 world_origin);
	public:
		owning_ptr<Shader> m_TerrainShader;
		owning_ptr<Shader> m_TerrainShader_DepthPP;
		owning_ptr<ComputeShader> m_ChunkGenerationShader;

		owning_ptr<ComputeShader> m_ShadowMapBaseMipGenerationShader;
		owning_ptr<ComputeShader> m_TextureOcclusionMipGenerationShader;

		std::unordered_map<Int2, TerrainChunk> m_ChunkTable;
		std::vector<TerrainChunk*> m_SortedChunks;
		owning_ptr<ShaderStorageBuffer> m_ChunkSSBO;

		owning_ptr<Texture3D> m_ShadowMap;
	};

} 