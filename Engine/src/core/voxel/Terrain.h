#pragma once

#include "rendering/Texture.h"

namespace Engine {
	
	class Shader;
	class ComputeShader;
	class ShaderStorageBuffer;

	struct TerrainChunk
	{
		static constexpr size_t Width = 512, Height = 128;

		VoxelMesh mesh;
		BindlessTexture3D bindless_texture;
		Int2 index{};
		Float3 position{};
	};

	class TerrainGenerator
	{
	public:
		TerrainGenerator();

		TerrainChunk& generate_chunk(Int2 planar_chunk_index);

		void resort_chunks(Int2 origin);
		void generate_shadowmap(Int2 origin);

		void render_terrain(const Matrix4& viewProjection, Float3 camera);
	private:
		void generate_occlusion_mips_for_texture(Texture3D* texture, size_t textureMipCount);
	public:
		owning_ptr<Shader> m_TerrainShader;
		owning_ptr<ComputeShader> m_ChunkGenerationShader;

		owning_ptr<ComputeShader> m_ShadowMapBaseMipGenerationShader;
		owning_ptr<ComputeShader> m_TextureOcclusionMipGenerationShader;

		std::unordered_map<Int2, TerrainChunk> m_ChunkTable;
		std::vector<TerrainChunk*> m_SortedChunks;
		owning_ptr<ShaderStorageBuffer> m_ChunkSSBO;

		owning_ptr<Texture3D> m_ShadowMap;
	};

} 