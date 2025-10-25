#include "pch.h"

#include "rendering/Texture.h"
#include "VoxelMesh.h"

#include <stb_image/stb_image.h>

#include "Terrain.h"

namespace Engine {	

	struct VoxelMeshData
	{
		uint8_t* voxels = nullptr;
		uint32_t palette[256]{};
	};

	// Generates 8-bit voxel data and corresponding palette from image pixels
	static VoxelMeshData process_voxel_image_data(uint8_t* pixels, uint32_t width, uint32_t height, uint32_t depth, uint32_t bytesPerPixel)
	{
		uint32_t voxelCount = width * height * depth;

		VoxelMeshData result;
		result.voxels = new uint8_t[voxelCount];

		size_t voxelIndex = 0;
		size_t paletteIndex = 1;
		for (uint32_t i = 0; i < voxelCount * 4; i += 4)
		{
			uint8_t r = pixels[i + 0];
			uint8_t g = pixels[i + 1];
			uint8_t b = pixels[i + 2];
			uint8_t a = pixels[i + 3];

			uint8_t& voxel_data = result.voxels[voxelIndex++];
			if (a == 0) {
				voxel_data = 0;
				continue;
			}

			uint32_t voxelColor = encode_rgba(a, b, g, r);

			// SEARCH Try find index of color in palette
			int32_t colorIndex = -1;
			for (size_t i = 0; i < paletteIndex; i++)
			{
				if (result.palette[i] == voxelColor)
				{
					colorIndex = i;
					break;
				}
			}

			voxel_data = colorIndex > 0 ? colorIndex : paletteIndex;
			if (colorIndex == -1) {

				result.palette[paletteIndex++] = voxelColor;
			}
		}

		// Fix it idfk
		uint8_t* remappedVoxels = new uint8_t[voxelCount];
		for (int z = 0; z < depth; z++)
		{
			for (int x = 0; x < width; x++)
			{
				for (int y = 0; y < height; y++)
				{
					int yidx = (height - 1) - y;
					remappedVoxels[(z * width * height) + (yidx * width) + x] = result.voxels[(((y * depth) + z) * width) + x];
				}
			}
		}

		delete[] result.voxels;
		result.voxels = remappedVoxels;

		return result;
	}

	// todo: fix ts
	static uint32_t s_MaterialIndex = 0;
	owning_ptr<Texture2D> VoxelMesh::s_MaterialPalette;

	VoxelMesh VoxelMesh::load_from_file(const std::filesystem::path& filepath)
	{	
		// TODO: better way of doin ts
		std::string path = filepath.string();
		auto sub = path.substr(path.find_last_of('_') + 1, path.find_last_of('.') - path.find_last_of('_') - 1);
		uint32_t sliceCount = strtol(sub.c_str(), nullptr, 0);

		VoxelMesh mesh;
		mesh.m_MaterialIndex = s_MaterialIndex++;

		// Load image
		Image image = image_load_from_file(filepath, false);
		uint32_t meshWidth = image.width;
		uint32_t meshHeight = sliceCount;
		uint32_t meshDepth = image.height / sliceCount;

		// Create texture
		auto& texture = mesh.m_Texture = Texture3D::create(meshWidth, meshHeight, meshDepth, TextureFormat::R8UI);
		texture->set_filter_mode(TextureFilterMode::Point);
		VoxelMeshData voxelData = process_voxel_image_data(image.pixels, meshWidth, meshHeight, meshDepth, image.channels);

		// Update palette
		s_MaterialPalette->set_data(voxelData.palette, 0, mesh.m_MaterialIndex, 0, 1);

		texture->set_data(voxelData.voxels);
		delete[] voxelData.voxels;

		return mesh;
	}

	VoxelMesh VoxelMesh::build_from_voxels(uint8_t* voxels, size_t width, size_t height, size_t depth, uint32_t* palette, uint32_t materialIndex)
	{
		VoxelMesh mesh;

		// Create texture
		auto& texture = mesh.m_Texture = Texture3D::create(width, height, depth, TextureFormat::R8UI);
		texture->set_filter_mode(TextureFilterMode::Point);
		texture->set_data(voxels);

		// palette
		mesh.m_MaterialIndex = materialIndex ? materialIndex : s_MaterialIndex++;
		s_MaterialPalette->set_data(palette, 0, materialIndex, 0, 1);

		return mesh;
	}

	void VoxelMesh::bind_palette(uint32_t slot)
	{
		if (!s_MaterialPalette) {
			static constexpr uint32_t PALETTE_MATS_COUNT = 16;
			s_MaterialPalette = Texture2D::create(256, PALETTE_MATS_COUNT, TextureFormat::RGBA8);
			
			// default palette
			{
				uint32_t default_palette[256]{};
				default_palette[0] = encode_rgba(0, 0, 0, 0);

				default_palette[1] = encode_rgba(Color(1.0f, 0.0f, 1.0f, 1.0f) / 1.0f);
				default_palette[2] = encode_rgba(Color(1.0f, 0.0f, 2.0f, 1.0f) / 2.0f);
				default_palette[3] = encode_rgba(Color(1.0f, 0.0f, 3.0f, 1.0f) / 3.0f);

				default_palette[4] = encode_rgba(Color(0.0f, 1.0f, 0.0f, 1.0f) / 1.0f);
				default_palette[5] = encode_rgba(Color(0.0f, 1.0f, 0.0f, 2.0f) / 2.0f);
				default_palette[6] = encode_rgba(Color(0.0f, 1.0f, 0.0f, 3.0f) / 3.0f);

				default_palette[7] = encode_rgba(Color(1.0f, 1.0f, 1.0f, 1.0f) / 1.0f);
				default_palette[8] = encode_rgba(Color(1.0f, 1.0f, 1.0f, 2.0f) / 2.0f);
				default_palette[9] = encode_rgba(Color(1.0f, 1.0f, 1.0f, 3.0f) / 3.0f);

				s_MaterialPalette->set_data(default_palette, 0, 0, 256, 1);
			}

			// heightmap palette
			{
				uint32_t debug_heightmap_palette[256]{};
				debug_heightmap_palette[0] = encode_rgba(0, 0, 0, 0);
				debug_heightmap_palette[1] = encode_rgba(66, 107, 255, 255);
				size_t num_heights = TerrainChunk::Height;
				for (size_t i = 2; i < num_heights; i++)
				{
					debug_heightmap_palette[i] = encode_rgba(Color(i, i, i, num_heights) / num_heights);
				}

				s_MaterialPalette->set_data(debug_heightmap_palette, 0, 1, 256, 1);
			}
		}

		s_MaterialPalette->bind(slot);
	}


}