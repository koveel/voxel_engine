#pragma once

namespace Engine {

	class Texture2D;
	class Texture3D;

	class VoxelMesh
	{
	public:
		VoxelMesh() = default;

		static VoxelMesh load_from_file(const std::filesystem::path& filepath);
		static VoxelMesh build_from_voxels(uint8_t* voxels, size_t w, size_t h, size_t d, uint32_t* palette, uint32_t materialIndex = 0);

		uint32_t m_MaterialIndex = 0;

		owning_ptr<Texture3D> m_Texture;
		static owning_ptr<Texture2D> s_MaterialPalette;

		static void bind_palette(uint32_t slot);
	private:
		uint32_t m_FilledVoxelCount = 0;
	};

}