#include "pch.h"

#include "Utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

namespace Engine {

	void image_save_jpg_to_file(const std::filesystem::path& path, uint32_t width, uint32_t height, const void* pixels)
	{
		stbi_flip_vertically_on_write(1);
		stbi_write_jpg(path.string().c_str(), width, height, 3, pixels, 100);
	}

	void image_save_png_to_file(const std::filesystem::path& path, uint32_t width, uint32_t height, const uint8_t* pixels)
	{
		stbi_flip_vertically_on_write(1);
		stbi_write_png(path.string().c_str(), width, height, 4, pixels, 0);
	}

	Image image_load_from_file(const std::filesystem::path& path, bool flip)
	{
		Image image;

		int width = 0, height = 0, channels = 0;
		stbi_set_flip_vertically_on_load(flip);
		image.pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
		ASSERT(image.pixels && "failed to load image file");

		image.width = width;
		image.height = height;
		image.channels = channels;

		return image;
	}

	Image::~Image()
	{
		stbi_image_free(pixels);
	}

	void image_swap_channels(uint8_t* rgba_dest, const uint8_t* abgr_src, size_t count)
	{
		/*
		[0, 1, 2, 3]
		*/
	
		for (size_t i = 0; i < count; i += 4)
		{
			rgba_dest[i + 0] = abgr_src[i + 3];
			rgba_dest[i + 1] = abgr_src[i + 2];
			rgba_dest[i + 2] = abgr_src[i + 1];
			rgba_dest[i + 3] = abgr_src[i + 0];
		}
	}

	//void remap_24bpp_to_32bpp(uint8_t* dest32, const uint8_t* source24, uint32_t width, uint32_t height)
	//{
	//	constexpr uint32_t DestChannels = 4, SrcChannels = 3;
	//
	//	uint32_t destIndex = 0;
	//	for (uint32_t i = 0; i < width * height * SrcChannels; i += SrcChannels)
	//	{
	//		uint8_t r = source24[i];
	//		uint8_t g = source24[i + 1];
	//		uint8_t b = source24[i + 2];
	//
	//		dest32[destIndex] = r;
	//		dest32[destIndex + 1] = g;
	//		dest32[destIndex + 2] = b;
	//		dest32[destIndex + 3] = 255;
	//		destIndex += 4;
	//	}
	//}

	int find_nth_index_of_char(const std::string& string, size_t idx, char c)
	{
		size_t n = 0;
		for (size_t i = 0; i < string.length(); i++)
		{
			if (string[i] == c && n++ == idx)
				return i;
		}

		return -1;
	}


}