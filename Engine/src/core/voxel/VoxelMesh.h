#pragma once

namespace Engine {
	
	class VoxelMesh
	{
	public:
		VoxelMesh() = default;

		static VoxelMesh load_from_file(const std::filesystem::path& filepath);

		uint32_t m_MaterialIndex = 0;
		std::unique_ptr<Texture3D> m_Texture;

		static std::unique_ptr<Texture2D> s_MaterialPalette;
	};

}