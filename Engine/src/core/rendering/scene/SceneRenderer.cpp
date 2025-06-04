#include "pch.h"

#include "rendering/Texture.h"
#include "rendering/Shader.h"
#include "rendering/Graphics.h"

#include "SceneRenderer.h"

#include "math/Math.h"
#include <glm/gtx/matrix_decompose.hpp>

namespace Engine {

	std::unique_ptr<Texture3D> SceneRenderer::s_ShadowMap;
	Float3 SceneRenderer::s_CameraPosition = Float3(0.0f);
	Matrix4 SceneRenderer::s_View = Matrix4(1.0f);
	Matrix4 SceneRenderer::s_Projection = Matrix4(1.0f);
	std::unique_ptr<Shader> SceneRenderer::s_VoxelMeshShader;

	void SceneRenderer::begin_frame(const Camera& camera, const Transformation& transform)
	{
		s_View = glm::inverse(transform.get_transform());
		s_CameraPosition = transform.Position;
		s_Projection = camera.get_projection();
	}

	static constexpr float VoxelScaleMeters = 0.1f;

	void SceneRenderer::draw_voxel_entity(const VoxelEntity& entity)
	{
		const auto& texture = entity.Mesh.m_Texture;
		const auto& transform = entity.Transform;

		Matrix4 rotation = transform.get_rotation();
		Matrix4 transformation = transform.get_transform();

		s_VoxelMeshShader->bind();
		s_VoxelMeshShader->set_int3("u_VoxelDimensions", texture->get_dimensions());
		s_VoxelMeshShader->set_int("u_MaterialIndex", entity.Mesh.m_MaterialIndex);

		s_VoxelMeshShader->set_float3("u_OBBCenter", transform.Position);
		s_VoxelMeshShader->set_matrix("u_OBBOrientation", glm::inverse(rotation));

		s_VoxelMeshShader->set_float3("u_CameraPosition", s_CameraPosition);
		s_VoxelMeshShader->set_matrix("u_Transformation", transformation);

		texture->bind();
		VoxelMesh::s_MaterialPalette->bind(1);
		Graphics::draw_cube();
	}

	void SceneRenderer::draw_shadow_map()
	{
		Texture3D* texture = SceneRenderer::get_shadow_map();
		Int3 voxelDimensions = texture->get_dimensions();

		Matrix4 rotation = Matrix4(1.0f);
		Matrix4 transformation = glm::scale(glm::mat4(1.0f), (Float3)voxelDimensions * VoxelScaleMeters);

		s_VoxelMeshShader->bind();
		s_VoxelMeshShader->set_float3("u_OBBCenter", Float3(0.0f, 0.0f, 0.0f));
		s_VoxelMeshShader->set_matrix("u_OBBOrientation", glm::inverse(rotation));
		s_VoxelMeshShader->set_int3("u_VoxelDimensions", voxelDimensions);
		s_VoxelMeshShader->set_int("u_MaterialIndex", 0);

		s_VoxelMeshShader->set_float3("u_CameraPosition", s_CameraPosition);
		s_VoxelMeshShader->set_matrix("u_Transformation", transformation);

		texture->bind();
		VoxelMesh::s_MaterialPalette->bind(1);
		Graphics::draw_cube();
	}

	void SceneRenderer::generate_shadow_map(const std::vector<VoxelEntity*>& entities)
	{
		uint32_t MapWidth = 250;
		uint32_t MapHeight = 250;
		uint32_t MapDepth = 250;
		Int3 MapDimensions = { MapWidth, MapHeight, MapDepth };
		uint32_t MapArea = MapWidth * MapHeight * MapDepth;

		static uint8_t* shadowMapPixels = new uint8_t[MapArea]; // TODO: delete dat shit
		if (!s_ShadowMap)
		{
			s_ShadowMap = Texture3D::create(MapWidth, MapHeight, MapDepth, TextureFormat::R8);
		}
		memset(shadowMapPixels, 0, MapArea);

		for (VoxelEntity* entity : entities)
		{
			const auto& texture = entity->Mesh.m_Texture;

			Int3 dimensions = texture->get_dimensions();

			Matrix4 volumeRotation = entity->Transform.get_rotation();
			Float3 volumeCenter = glm::inverse(volumeRotation) * Float4(entity->Transform.Position, 1.0f);

			for (int z = 0; z < dimensions.z; z++)
			{
				for (int x = 0; x < dimensions.x; x++)
				{
					for (int y = 0; y < dimensions.y; y++)
					{
						uint8_t voxelData = texture->m_PixelData[(z * dimensions.x * dimensions.y) + (y * dimensions.x) + x];
						if (voxelData == 0) // Empty voxel
							continue;

						// Center of Voxel normalized among volume (-0.5 - 0.5)
						Float3 voxelCenterNormalized = {
							((x + 0.5f) / dimensions.x) - 0.5f,
							((y + 0.5f) / dimensions.y) - 0.5f,
							((z + 0.5f) / dimensions.z) - 0.5f,
						};

						// Transform that shit
						Float3 volumeCenterScaled = volumeCenter + voxelCenterNormalized * (Float3)dimensions * 0.1f;
						volumeCenterScaled = volumeRotation * Float4(volumeCenterScaled / 0.1f, 1.0f);

						// Corresponding cell in environment
						Int3 cellIndex = (Int3)glm::floor(volumeCenterScaled + 0.05f) + MapDimensions / 2;

						// Bounds check
						if (cellIndex.x < 0 || cellIndex.y < 0 || cellIndex.z < 0 ||
							cellIndex.x > MapWidth - 1 || cellIndex.y > MapHeight - 1 || cellIndex.z > MapDepth - 1)
							continue;

						shadowMapPixels[(cellIndex.z * MapHeight * MapDepth) + (cellIndex.y * MapWidth) + (cellIndex.x)] = 1;
					}
				}
			}

			s_ShadowMap->set_data(shadowMapPixels);
		}
	}

}