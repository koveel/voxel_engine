#include "pch.h"

#include <glad/glad.h>

#include "VoxelMesh.h"
#include "Terrain.h"

#include "rendering/Shader.h"
#include "rendering/Texture.h"
#include "rendering/scene/SceneRenderer.h"

#include <simplex_noise/SimplexNoise.h>

namespace Engine {


	TerrainGenerator::TerrainGenerator()
	{
		//m_HeightMapSSBO = ShaderStorageBuffer::create(nullptr, TerrainChunk::Width * TerrainChunk::Width * sizeof(float));
		m_ChunkGenerationShader = ComputeShader::create("resources/shaders/compute/Compute_GenerateTerrain.glsl");
		m_ChunkGenerationShader->set_int3("u_ChunkDimensions", { TerrainChunk::Width, TerrainChunk::Height, TerrainChunk::Width });
	}

	TerrainChunk TerrainGenerator::generate_chunk(Int2 planar_chunk_index)
	{
		// Generate chunk
		TerrainChunk chunk{};

		auto texture = Texture3D::create(TerrainChunk::Width, TerrainChunk::Height, TerrainChunk::Width, TextureFormat::R8UI, 1);

		// direct create and update shadow map (for now)
		auto& shadowMap = SceneRenderer::s_ShadowMap;
		if (!shadowMap)
			shadowMap = Texture3D::create(TerrainChunk::Width, TerrainChunk::Height, TerrainChunk::Width, TextureFormat::R8UI, 3);

		texture->bind_as_image(0, TextureAccessMode::Write, 0);
		shadowMap->bind_as_image(1, TextureAccessMode::Write, 0);

		chunk.position = Float3(planar_chunk_index.x, 0.0f, planar_chunk_index.y) * (float)(TerrainChunk::Width);
		chunk.position *= VoxelScaleMeters;
		m_ChunkGenerationShader->set_float3("u_ChunkPositionWorld", chunk.position);
		
		constexpr uint32_t LocalSizeInShader = 4;
		m_ChunkGenerationShader->dispatch(TerrainChunk::Width / LocalSizeInShader, TerrainChunk::Height / LocalSizeInShader, TerrainChunk::Width / LocalSizeInShader);
		Graphics::memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Generate mips...

		chunk.index = planar_chunk_index;
		chunk.mesh.m_Texture = std::move(texture);

		return chunk;
	}

	//void TerrainGenerator::regenerate_chunk(VoxelMesh& mesh, Float3 noiseOffset)
	//{
	//	auto& texture = mesh.m_Texture;
	//	auto& shadowMap = SceneRenderer::s_ShadowMap;
	//
	//	static uint8_t v[TerrainChunk::Width * TerrainChunk::Height * TerrainChunk::Width]{};
	//	texture->set_data(v);
	//	shadowMap->set_data(v);
	//
	//	texture->bind_as_image(0, TextureAccessMode::Write, 0);
	//	shadowMap->bind_as_image(1, TextureAccessMode::Write, 0);
	//
	//	//m_ChunkGenerationShader->set_float3("u_NoiseOffset", noiseOffset);
	//
	//	loat3 planar_world = Float3(planar_chunk_index.x, 0.0f, planar_chunk_index.y) / (float)TerrainChunk::Width;
	//	m_ChunkGenerationShader->set_float3("u_ChunkPositionWorld", planar_world);
	//
	//	constexpr uint32_t LocalSizeInShader = 4;
	//	m_ChunkGenerationShader->dispatch(TerrainChunk::Width / LocalSizeInShader, TerrainChunk::Height / LocalSizeInShader, TerrainChunk::Width / LocalSizeInShader);
	//
	//	// Generate mips...
	//}

}