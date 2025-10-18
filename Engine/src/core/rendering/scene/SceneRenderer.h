#pragma once

#include "utils/Camera.h"
#include "voxel/VoxelEntity.h"

namespace Engine {

	class Shader;
	class Texture3D;

	struct TracedRay
	{
		Float3 direction{};
		Float3 hitpoint{};
		bool hit = false;
		float t = 0.0f;
		uint8_t sample = 0;
		Int3 cell{};
	};

	class SceneRenderer
	{
	public:
		static void begin_frame(const Camera& camera, const Transformation& view);
		static void draw_voxel_entity(const VoxelEntity& entity);
		static void draw_shadow_map();

		static void generate_shadow_map(const std::vector<VoxelEntity*>& entities);

		static TracedRay trace_ray_through_scene(Float3 origin, Float3 direction, float maxDistanceMeters);

		static Texture3D* get_shadow_map() { return s_ShadowMap.get(); }
	private:
		static uint8_t sample_shadow_map(Int3 cell);
	private:
		static Float3 s_CameraPosition;
		static Matrix4 s_View, s_Projection;
		static std::unique_ptr<Texture3D> s_ShadowMap;
	public:
		static std::unique_ptr<Shader> s_VoxelMeshShader;
	};

}