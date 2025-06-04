#pragma once

#include "utils/Camera.h"
#include "voxel/VoxelEntity.h"

class Shader;
class Texture3D;

namespace Engine {

	class SceneRenderer
	{
	public:
		static void begin_frame(const Camera& camera, const Transformation& view);
		static void draw_voxel_entity(const VoxelEntity& entity);
		static void draw_shadow_map();

		static void generate_shadow_map(const std::vector<VoxelEntity*>& entities);

		static Texture3D* get_shadow_map() { return s_ShadowMap.get(); }
	private:
		static Float3 s_CameraPosition;
		static Matrix4 s_View, s_Projection;
		static std::unique_ptr<Texture3D> s_ShadowMap;
	public:
		static std::unique_ptr<Shader> s_VoxelMeshShader;
	};

}