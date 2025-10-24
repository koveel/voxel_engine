#pragma once

namespace Engine {

	class Texture2D;
	class Texture3D;

	class VoxelMesh
	{
	public:
		VoxelMesh() = default;
		//VoxelMesh(VoxelMesh&& other) noexcept
		//	: m_Texture(std::move(other.m_Texture)), m_MaterialIndex(other.m_MaterialIndex)
		//{}
		//VoxelMesh& operator=(VoxelMesh&& other)
		//{
		//	m_Texture = std::move(other.m_Texture);
		//	m_MaterialIndex = other.m_MaterialIndex;
		//}

		static VoxelMesh load_from_file(const std::filesystem::path& filepath);
		static VoxelMesh build_from_voxels(uint8_t* voxels, size_t w, size_t h, size_t d, uint32_t* palette, uint32_t materialIndex = 0);

		static void bind_palette(uint32_t slot);
	public:
		uint32_t m_MaterialIndex = 0;
		owning_ptr<Texture3D> m_Texture;

		static owning_ptr<Texture2D> s_MaterialPalette;
	};

}