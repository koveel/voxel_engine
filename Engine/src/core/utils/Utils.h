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

	//void image_swap_channels(uint8_t* rgba_dest, const uint8_t* abgr_src, size_t count);
	//void remap_24bpp_to_32bpp(uint8_t* dest32, const uint8_t* source24, uint32_t width, uint32_t height);

	int find_nth_index_of_char(const std::string& string, size_t n, char c);

	static uint32_t encode_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
		return r | (g << 8) | (g << 16) | (a << 24);
	}

	static uint32_t encode_rgba(Color color) {
		uint8_t r = static_cast<uint8_t>(color.r * 255.0f);
		uint8_t g = static_cast<uint8_t>(color.g * 255.0f);
		uint8_t b = static_cast<uint8_t>(color.b * 255.0f);
		uint8_t a = static_cast<uint8_t>(color.a * 255.0f);
		return encode_rgba(r, g, b, a);
	}

	static size_t flatten_index_3d(Int3 i, Int3 dimensions)
	{
		return i.x + (i.z * dimensions.z * dimensions.y) + (i.y * dimensions.z);
	}

	static size_t flatten_index_2d(Int2 i, Int2 dimensions)
	{
		return i.x + (i.y * dimensions.x);
	}

}