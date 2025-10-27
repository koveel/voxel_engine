#include "pch.h"

#include <glad/glad.h>

#include "VoxelMesh.h"
#include "Terrain.h"

#include "rendering/Shader.h"
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

	TerrainGenerator::TerrainGenerator()
	{
		m_ChunkGenerationShader = ComputeShader::create("resources/shaders/compute/Compute_GenerateTerrain.glsl");

		m_ShadowMapMipGenerationShader = ComputeShader::create("resources/shaders/compute/Compute_GenShadowmapMip.glsl");
		m_ShadowMapBaseMipGenerationShader = ComputeShader::create("resources/shaders/compute/Compute_GenShadowmapBase.glsl");

		Int3 chunk_dimensions = Int3(TerrainChunk::Width, TerrainChunk::Height, TerrainChunk::Width);
		m_ChunkGenerationShader->set("u_ChunkDimensions", chunk_dimensions);
		m_ShadowMapBaseMipGenerationShader->set("u_ChunkDimensions", chunk_dimensions);

		m_ShadowMap = Texture3D::create(ShadowMapWidth, ShadowMapHeight, ShadowMapWidth, TextureFormat::R8UI, ShadowMapNumMips);
	}

	TerrainChunk& TerrainGenerator::generate_chunk(Int2 planar_chunk_index)
	{
		// Generate chunk
		TerrainChunk chunk{};

		auto texture = Texture3D::create(TerrainChunk::Width, TerrainChunk::Height, TerrainChunk::Width, TextureFormat::R8UI, 1);

		texture->bind_as_image(0, TextureAccessMode::Write, 0);

		Float3 position = Float3(planar_chunk_index.x, 0.0f, planar_chunk_index.y) * (float)(TerrainChunk::Width);
		position *= VoxelScaleMeters;
		m_ChunkGenerationShader->set("u_ChunkPositionWorld", position);
		
		constexpr uint32_t LocalSizeInShader = 4;
		m_ChunkGenerationShader->dispatch(TerrainChunk::Width / LocalSizeInShader, TerrainChunk::Height / LocalSizeInShader, TerrainChunk::Width / LocalSizeInShader);
		Graphics::memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		
		// Generate mips		

		chunk.position = position;
		chunk.index = planar_chunk_index;
		chunk.mesh.m_Texture = std::move(texture);
		chunk.mesh.m_MaterialIndex = 1;

		// TODO: streaming (along w chunks)
		chunk.bindless_image = chunk.mesh.m_Texture;
		chunk.bindless_image.activate(TextureAccessMode::Read);

		return m_ChunkTable[chunk.index] = std::move(chunk);
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
			uint64_t handle = chunk.bindless_image.get_handle();
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

		LOG("generate shadowmap mip: mip 0 ({}, {}, {})", ShadowMapWidth, ShadowMapHeight, ShadowMapWidth);
		m_ShadowMapBaseMipGenerationShader->dispatch(Width, Height, Width);

		Graphics::memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		// Generate mips
		for (size_t mip = 0; mip < ShadowMapNumMips - 1; mip++)
		{
			m_ShadowMap->bind_as_image(0, TextureAccessMode::Read, mip);
			m_ShadowMap->bind_as_image(1, TextureAccessMode::Write, mip + 1);

			size_t writeMipWidth = ShadowMapWidth / 2 >> mip;
			size_t writeMipHeight = ShadowMapHeight / 2 >> mip;

			constexpr size_t LocalSizeInShader = 4;
			size_t dispatch_x = writeMipWidth / LocalSizeInShader;
			size_t dispatch_y = writeMipHeight / LocalSizeInShader;
			m_ShadowMapMipGenerationShader->dispatch(dispatch_x, dispatch_y, dispatch_x);

			LOG("generate shadowmap mip: mip {} ({}, {}, {})", mip + 1, writeMipWidth, writeMipHeight, writeMipWidth);
			Graphics::memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
	}

}