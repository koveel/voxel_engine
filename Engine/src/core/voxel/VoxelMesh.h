#pragma once

namespace Engine {

	class Texture2D;
	class Texture3D;

	class VoxelMesh
	{
	public:
		VoxelMesh() = default;

		static VoxelMesh load_from_file(const std::filesystem::path& filepath);

		uint32_t m_MaterialIndex = 0;

		owning_ptr<Texture3D> m_Texture;
		static owning_ptr<Texture2D> s_MaterialPalette;
	private:
		uint32_t m_FilledVoxelCount = 0;
	};

}