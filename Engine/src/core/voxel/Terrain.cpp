#include "pch.h"

#include <glad/glad.h>

#include "VoxelMesh.h"
#include "Terrain.h"

#include "rendering/Shader.h"
#include "rendering/Buffer.h"
#include "rendering/Texture.h"
#include "rendering/scene/SceneRenderer.h"

#include <simplex_noise/SimplexNoise.h>

namespace Engine {

	static constexpr size_t ShadowMapNumMips = 3;
	static constexpr size_t ShadowMapNumChunks = 3;
	static constexpr size_t ShadowMapPackFactor = 2;
	static constexpr size_t ShadowMapWidth = (TerrainChunk::Width * ShadowMapNumChunks) / ShadowMapPackFactor;
	static constexpr size_t ShadowMapHeight = TerrainChunk::Height / ShadowMapPackFactor;

	static constexpr size_t ShadowMapMemorySize = ShadowMapWidth * ShadowMapHeight * ShadowMapWidth;
		
	struct alignas(16) ChunkInstanceData
	{
		Matrix4 transformation;
		uint64_t voxel_texture;
	};

	TerrainGenerator::TerrainGenerator()
	{
		m_TerrainShader = Shader::create("resources/shaders/TerrainShader.glsl");
		m_ChunkGenerationShader = ComputeShader::create("resources/shaders/compute/Compute_GenerateTerrain.glsl");

		m_TextureOcclusionMipGenerationShader = ComputeShader::create("resources/shaders/compute/Compute_GenOcclusionMip.glsl");
		m_ShadowMapBaseMipGenerationShader = ComputeShader::create("resources/shaders/compute/Compute_GenShadowmapBase.glsl");

		Int3 chunk_dimensions = Int3(TerrainChunk::Width, TerrainChunk::Height, TerrainChunk::Width);
		m_ChunkGenerationShader->set("u_ChunkDimensions", chunk_dimensions);
		m_ShadowMapBaseMipGenerationShader->set("u_ChunkDimensions", chunk_dimensions);

		m_ChunkSSBO = ShaderStorageBuffer::create(nullptr, sizeof(ChunkInstanceData) * 128);

		m_ShadowMap = Texture3D::create(ShadowMapWidth, ShadowMapHeight, ShadowMapWidth, TextureFormat::R8UI, ShadowMapNumMips);
	}

	TerrainChunk& TerrainGenerator::generate_chunk(Int2 planar_chunk_index)
	{
		// Generate chunk
		TerrainChunk chunk{};

		auto texture = Texture3D::create(TerrainChunk::Width, TerrainChunk::Height, TerrainChunk::Width, TextureFormat::R8UI, 3);

		texture->bind_as_image(0, TextureAccessMode::Write, 0);

		Float3 position = Float3(planar_chunk_index.x, 0.0f, planar_chunk_index.y) * (float)(TerrainChunk::Width);
		position *= VoxelScaleMeters;
		m_ChunkGenerationShader->set("u_ChunkPositionWorld", position);
		
		constexpr uint32_t LocalSizeInShader = 4;
		m_ChunkGenerationShader->dispatch(TerrainChunk::Width / LocalSizeInShader, TerrainChunk::Height / LocalSizeInShader, TerrainChunk::Width / LocalSizeInShader);
		Graphics::memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
		
		// Generate mips
		generate_occlusion_mips_for_texture(texture.get(), 3);

		chunk.position = position;
		chunk.index = planar_chunk_index;
		chunk.mesh.m_Texture = std::move(texture);
		chunk.mesh.m_MaterialIndex = 1;

		//chunk.bindless_image = chunk.mesh.m_Texture->get_bindless_image(0);
		//chunk.bindless_image.activate(TextureAccessMode::Read);
		chunk.bindless_texture = chunk.mesh.m_Texture->get_bindless_texture();
		chunk.bindless_texture.activate();

		return m_ChunkTable[chunk.index] = std::move(chunk);
	}	

	void TerrainGenerator::resort_chunks(Int2 origin)
	{
		m_SortedChunks.clear();
		m_SortedChunks.reserve(25);

		static ChunkInstanceData instance_data[256]{};

		for (auto& pair : m_ChunkTable)
		{
			m_SortedChunks.push_back(&pair.second);
		}

		std::sort(m_SortedChunks.begin(), m_SortedChunks.end(), [origin](TerrainChunk* a, TerrainChunk* b)
		{
			Int2 indexA = a->index, indexB = b->index;

			Int2 dA = indexA - origin;
			Int2 dB = indexB - origin;
			int len0 = dA.x * dA.x + dA.y * dA.y;
			int len1 = dB.x * dB.x + dB.y * dB.y;

			if (len0 != len1)
				return len0 < len1;

			return indexA.x != indexB.x ? indexA.x < indexB.x : indexA.y < indexB.y;
		});

		ChunkInstanceData* chunk_ptr = instance_data;
		for (TerrainChunk* chunk : m_SortedChunks)
		{
			chunk_ptr->transformation = Transformation(chunk->position, {}, Float3(chunk->mesh.m_Texture->get_dimensions()) * 0.1f).get_transform();
			chunk_ptr->voxel_texture = chunk->bindless_texture.get_handle();
			chunk_ptr++;
		}

		size_t instance_data_size = sizeof(ChunkInstanceData) * m_SortedChunks.size();
		m_ChunkSSBO->set_data(instance_data, instance_data_size);
	}

	void TerrainGenerator::generate_occlusion_mips_for_texture(Texture3D* texture, size_t textureMipCount)
	{
		size_t baseWidth = texture->get_width();
		size_t baseHeight = texture->get_height();

		m_TextureOcclusionMipGenerationShader->bind();
		// Generate mips
		for (size_t mip = 0; mip < textureMipCount - 1; mip++)
		{
			texture->bind_as_image(0, TextureAccessMode::Read, mip);
			texture->bind_as_image(1, TextureAccessMode::Write, mip + 1);

			size_t writeMipWidth = baseWidth / 2 >> mip;
			size_t writeMipHeight = baseHeight / 2 >> mip;

			constexpr size_t LocalSizeInShader = 4;
			size_t dispatch_x = writeMipWidth / LocalSizeInShader;
			size_t dispatch_y = writeMipHeight / LocalSizeInShader;
			m_TextureOcclusionMipGenerationShader->dispatch(dispatch_x, dispatch_y, dispatch_x);

			Graphics::memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
	}

	void TerrainGenerator::generate_shadowmap(Int2 center_chunk)
	{
		// bind them chunks boy (DEBUG DEBUG)
		Int2 target_chunk_indices[ShadowMapNumChunks * ShadowMapNumChunks] =
		{
			center_chunk + Int2(-1, -1), center_chunk + Int2(0, -1), center_chunk + Int2(1, -1),
			center_chunk + Int2(-1,  0), center_chunk, center_chunk + Int2(1, 0),
			center_chunk + Int2(-1,  1), center_chunk + Int2(0, 1), center_chunk + Int2(1, 1),
		};

		uint32_t chunk_count = 0;
		uint64_t bindless_handles[9]{};
		for (size_t i = 0; i < std::size(target_chunk_indices); i++)
		{
			Int2 index = target_chunk_indices[i];
			if (!m_ChunkTable.count(index))
				continue;

			TerrainChunk& chunk = m_ChunkTable[index];
			uint64_t handle = chunk.bindless_texture.get_handle();
			bindless_handles[i] = handle;
			chunk_count++;
		}

		m_ShadowMap->bind_as_image(0, TextureAccessMode::Write);
		// TODO: smth like vector with sbo instead of std::init list ?
		m_ShadowMapBaseMipGenerationShader->set("u_ChunkHandles", std::initializer_list<uint64_t>{
			bindless_handles[0], bindless_handles[1], bindless_handles[2],
				bindless_handles[3], bindless_handles[4], bindless_handles[5],
				bindless_handles[6], bindless_handles[7], bindless_handles[8],
		});
		m_ShadowMapBaseMipGenerationShader->set("u_ChunkCount", chunk_count);

		constexpr size_t LocalSizeInShader = 4;
		constexpr size_t Width = ShadowMapWidth / LocalSizeInShader;
		constexpr size_t Height = ShadowMapHeight / LocalSizeInShader;

		m_ShadowMapBaseMipGenerationShader->dispatch(Width, Height, Width);

		Graphics::memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		// Generate mips
		generate_occlusion_mips_for_texture(m_ShadowMap.get(), ShadowMapNumMips);
	}

	void TerrainGenerator::render_terrain(const Matrix4& viewProj, Float3 camera)
	{
		m_TerrainShader->bind();
		m_ChunkSSBO->bind(0);
		m_ShadowMap->bind(1);

		m_TerrainShader->set("u_MaterialIndex", 1);
		//m_TerrainShader->set("u_MipLevel", 0);

		m_TerrainShader->set("u_CameraPosition", camera);
		m_TerrainShader->set("u_ViewProjection", viewProj);

		VoxelMesh::bind_palette(3);

		Graphics::draw_cubes_instanced(m_SortedChunks.size());
	}

}