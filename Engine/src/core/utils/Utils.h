#pragma once

namespace Engine {

	// temp wrapper around some pixels
	struct Image
	{
		uint32_t width = 0, height = 0, channels = 0;
		uint8_t* pixels = nullptr;

		~Image();
	};

	void image_save_jpg_to_file(const std::filesystem::path& path, uint32_t width, uint32_t height, const void* pixels);
	void image_save_png_to_file(const std::filesystem::path& path, uint32_t width, uint32_t height, const uint8_t* pixels);

	Image image_load_from_file(const std::filesystem::path& path, bool flipVertically = true);

	void image_swap_channels(uint8_t* abgr_dest, const uint8_t* rgba_src, uint32_t size);
	void remap_24bpp_to_32bpp(uint8_t* dest32, const uint8_t* source24, uint32_t width, uint32_t height);

	int find_nth_index_of_char(const std::string& string, size_t n, char c);

	static int flatten_index_3d(Int3 i, Int3 dimensions)
	{
		return (i.z * dimensions.z * dimensions.y) + (i.y * dimensions.z) + i.x;
	}

}