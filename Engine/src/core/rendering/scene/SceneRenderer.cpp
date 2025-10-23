#include "pch.h"

#include "rendering/Texture.h"
#include "rendering/Shader.h"
#include "rendering/Graphics.h"

#include "SceneRenderer.h"

namespace Engine {

	owning_ptr<Texture3D> SceneRenderer::s_ShadowMap;
	owning_ptr<Shader> SceneRenderer::s_VoxelMeshShader;

	Matrix4 SceneRenderer::s_View = Matrix4(1.0f);
	Matrix4 SceneRenderer::s_Projection = Matrix4(1.0f);	

	void SceneRenderer::begin_frame(const Camera& camera, const Transformation& transform)
	{
		s_View = glm::inverse(transform.get_transform());
		s_Projection = camera.get_projection();		
	}

	void SceneRenderer::draw_entities(const std::vector<VoxelEntity*>& entities)
	{
		for (auto& e : entities)
			draw_voxel_entity(*e);
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

		s_VoxelMeshShader->set_matrix("u_Transformation", transformation);

		texture->bind();
		VoxelMesh::s_MaterialPalette->bind(1);
		Graphics::draw_cube();
	}

	void SceneRenderer::generate_shadow_map(const std::vector<VoxelEntity*>& entities)
	{
		uint32_t MapWidth = 250;
		uint32_t MapHeight = 150;
		uint32_t MapDepth = 250;
		Int3 MapDimensions = { MapWidth, MapHeight, MapDepth };
		uint32_t MapArea = MapWidth * MapHeight * MapDepth;

		static uint8_t* shadowMapPixels = new uint8_t[MapArea]; // TODO: delete dat shit
		if (!s_ShadowMap)
		{
			s_ShadowMap = Texture3D::create(MapWidth, MapHeight, MapDepth, TextureFormat::R8UI, 3);
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

						//size_t index = cellIndex.x + (cellIndex.z * MapHeight * MapWidth) + (cellIndex.y * MapWidth);
						size_t index = flatten_index_3d(cellIndex, {MapWidth, MapHeight, MapDepth});
						shadowMapPixels[index] = 1;
					}
				}
			}

			s_ShadowMap->set_data(shadowMapPixels);
			s_ShadowMap->generate_mips();
		}
	}

	TracedRay SceneRenderer::trace_ray_through_scene(Float3 origin, Float3 direction, float maxDistanceMeters)
	{
		constexpr float VoxelScale = 0.1f;
		Int3 mapDimensions = s_ShadowMap->get_dimensions();

		Float3 worldspaceExtents = (Float3(mapDimensions) * VoxelScale) / 2.0f;
		Float3 boundsMin = -worldspaceExtents;

		Float3 voxelsPerUnit = Float3(1.0f / VoxelScale);

		Float3 entry = (origin - boundsMin) * voxelsPerUnit;
		Float3 step = glm::sign(direction);
		Int3 iStep = Int3(step);

		Float3 delta = glm::abs(1.0f / direction);
		Int3 pos = Int3(glm::clamp(glm::floor(entry), Float3(0.0f), Float3(mapDimensions) - 1.0f));
		Float3 tMax = (Float3(pos) - entry + glm::max(step, 0.0f)) / direction;

		TracedRay result;
		result.direction = direction;
		 
		// early exit
		if (glm::any(glm::lessThan(pos, Int3(0))) || glm::any(glm::greaterThan(pos, mapDimensions))) {
			result.t = maxDistanceMeters;
			return result;
		}

		int maxSteps = (int)(maxDistanceMeters / VoxelScale);
		for (int i = 0; i < maxSteps; i++) {
			Int3 uv = 
			{ 
				pos.x,
				pos.y,
				pos.z,
			};
			uint8_t sample = sample_shadow_map(uv);
			if (sample != 0) {
				// Find which axis we just crossed
				int axis = (tMax.x < tMax.y) ? ((tMax.x < tMax.z) ? 0 : 2) : ((tMax.y < tMax.z) ? 1 : 2);
				result.t = (tMax[axis] - delta[axis]) * VoxelScale;
				result.hitpoint = origin + direction * result.t;
				result.sample = sample;
				result.hit = true;
				result.cell = pos - mapDimensions / 2;
				return result;
			}

			if (tMax.x < tMax.y) {
				if (tMax.x < tMax.z) {
					// Step X
					pos.x += iStep.x;
					tMax.x += delta.x;
				}
				else {
					// Step Z
					pos.z += iStep.z;
					tMax.z += delta.z;
				}
			}
			else {
				if (tMax.y < tMax.z) {
					// Step Y
					pos.y += iStep.y;
					tMax.y += delta.y;
				}
				else {
					// Step Z
					pos.z += iStep.z;
					tMax.z += delta.z;
				}
			}

			// exit map?? :(
			if (glm::any(glm::lessThan(pos, Int3(0))) || glm::any(glm::greaterThanEqual(pos, mapDimensions))) {
				break;
			}
		}

		result.t = float(maxDistanceMeters);
		return result;
	}

	uint8_t SceneRenderer::sample_shadow_map(Int3 cell)
	{
		size_t index = flatten_index_3d(cell, s_ShadowMap->get_dimensions());
		return s_ShadowMap->m_PixelData[index];
	}

}