#pragma once

namespace Engine {

	enum class TextureFormat
	{
		R8     = 0x8229,
		R16    = 0x822A,
		R32F   = 0x822E,
		RG8    = 0X822B,
		RGB8   = 0x8051,
		RGBA8  = 0x8058,
		RGB8S  = 0x8F96,
		RGBA8S = 0x8F97,
		A8     = 0x803C,

		Depth16 = 0x81A5,
		Depth24 = 0x81A6,
		Depth32 = 0x81A7,

		Depth24Stencil8 = 0x88F0,
	};

	enum class TextureFilterMode
	{
		Point = 0x2600,
		Linear = 0x2601,
	};

	enum class TextureWrapMode
	{
		Repeat = 0x2901,
		Clamp = 0x812F,
	};

	class Texture2D
	{
	private:
		Texture2D(uint32_t width, uint32_t height, TextureFormat format, uint32_t mips = 1);
	public:
		~Texture2D();

		void set_filter_mode(TextureFilterMode mode);
		void set_wrap_mode(TextureWrapMode mode);

		void set_data(const void* data, uint32_t x = 0, uint32_t y = 0, uint32_t width = 0, uint32_t height = 0, uint32_t mip = 0);

		uint32_t get_width() const { return m_Width; }
		uint32_t get_height() const { return m_Height; }
		Int2 get_dimensions() const { return Int2(m_Width, m_Height); }

		void bind(uint32_t slot = 0) const;
		uint32_t get_handle() const { return m_ID; }
		
		TextureFormat get_format() const { return (TextureFormat)m_InternalFormat; }

		static std::unique_ptr<Texture2D> load(const std::filesystem::path& filepath);
		static std::unique_ptr<Texture2D> create(uint32_t width, uint32_t height, TextureFormat format, uint32_t mips = 1);
	private:
		uint32_t m_ID = 0;
		uint32_t m_Width = 0, m_Height = 0;

		uint32_t m_InternalFormat, m_DataFormat;
	};

	class Texture3D
	{
	private:
		Texture3D(uint32_t width, uint32_t height, uint32_t depth, TextureFormat format, uint32_t mips = 1);
	public:
		~Texture3D();

		void set_filter_mode(TextureFilterMode mode);
		void set_wrap_mode(TextureWrapMode mode);
		void set_data(const void* data, uint32_t x = 0, uint32_t y = 0, uint32_t z = 0, uint32_t mip = 0);

		uint32_t get_width() const { return m_Width; }
		uint32_t get_height() const { return m_Height; }
		uint32_t get_depth() const { return m_Depth; }
		Int3 get_dimensions() const { return Int3(m_Width, m_Height, m_Depth); }

		void generate_mips();

		void bind(uint32_t slot = 0) const;
		uint32_t get_handle() const { return m_ID; }

		static std::unique_ptr<Texture3D> create(uint32_t width, uint32_t height, uint32_t depth, TextureFormat format, uint32_t mips = 1);

		uint8_t* m_PixelData = nullptr; // move
	private:
		uint32_t m_ID = 0;
		uint32_t m_Width = 0, m_Height = 0, m_Depth = 0;

		uint32_t m_InternalFormat, m_DataFormat;
	};

}