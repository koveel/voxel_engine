#include "pch.h"

#include <glad/glad.h>
#include "Texture.h"

#include <stb_image/stb_image.h>

namespace Engine {

	static GLenum gl_data_format_from_internal_format(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::A8:
		case TextureFormat::R8:
		case TextureFormat::R16:
		case TextureFormat::R32F:
		case TextureFormat::Depth16:
		case TextureFormat::Depth24:
		case TextureFormat::Depth32: return GL_RED;
		case TextureFormat::RG8:     return GL_RG;
		case TextureFormat::RGB8:    return GL_RGB;
		case TextureFormat::RGBA8:   return GL_RGBA;
		case TextureFormat::R8UI:    return GL_RED_INTEGER;
		}
	}
	
	static bool is_integer_format(TextureFormat format)
	{
		return format == TextureFormat::R8UI;
	}

	Texture2D::Texture2D(uint32_t width, uint32_t height, TextureFormat format, uint32_t mips)
		: m_Width(width), m_Height(height), m_InternalFormat(format)
	{	
		m_DataFormat = gl_data_format_from_internal_format(format);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_ID);
		glTextureStorage2D(m_ID, mips, (GLenum)format, m_Width, m_Height);
	}

	Texture2D::~Texture2D()
	{
		glDeleteTextures(1, &m_ID);
	}

	void Texture2D::bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_ID);
	}

	void Texture2D::bind_as_image(uint32_t slot, TextureAccessMode mode) const
	{
		glBindImageTexture(slot, m_ID, 0, GL_FALSE, 0, (GLenum)mode, (uint32_t)m_InternalFormat);
	}	

	void Texture2D::set_filter_mode(TextureFilterMode mode)
	{
		glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, (GLenum)mode);
		glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, (GLenum)mode);
	}

	void Texture2D::set_wrap_mode(TextureWrapMode mode)
	{
		glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, (GLenum)mode);
		glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, (GLenum)mode);
	}

	void Texture2D::set_data(const void* data, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t mip)
	{
		if (width == 0) width = m_Width;
		if (height == 0) height = m_Height;

		GLenum dataType;
		uint32_t unpackAlignment = 0;
		switch (m_DataFormat)
		{
		case GL_RG: {
			dataType = GL_UNSIGNED_SHORT;
			break;
		}
		case GL_RED: {
			dataType = GL_UNSIGNED_BYTE;
			break;
		}
		case GL_RED_INTEGER: {
			dataType = GL_UNSIGNED_BYTE;
			break;
		}
		case GL_RGB: {
			dataType = GL_UNSIGNED_BYTE;
			break;
		}
		case GL_RGBA: {
			dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
			break;
		}
		}

		glTextureSubImage2D(m_ID, mip, x, y, width, height, m_DataFormat, dataType, data);
	}

	void Texture2D::clear_to(const void* data)
	{
		glClearTexImage(m_ID, 0, m_DataFormat, GL_FLOAT, data);
	}

	owning_ptr<Texture2D> Texture2D::load(const std::filesystem::path& filepath)
	{
		Image image = image_load_from_file(filepath);
		TextureFormat format = image.channels == 4 ? TextureFormat::RGBA8 : TextureFormat::RGB8;
		
		auto result = owning_ptr<Texture2D>(new Texture2D(image.width, image.height, format));
		result->set_data(image.pixels);

		return result;
	}

	owning_ptr<Texture2D> Texture2D::create(uint32_t width, uint32_t height, TextureFormat format, uint32_t mips)
	{
		return owning_ptr<Texture2D>(new Texture2D(width, height, format, mips));
	}

	TextureView::TextureView(const owning_ptr<Texture2D>& texture, TextureFormat format)
		: m_InternalFormat(format)
	{
		glGenTextures(1, &m_ID);
		glTextureView(m_ID, GL_TEXTURE_2D, texture->get_handle(), (uint32_t)m_InternalFormat, 0, 1, 0, 1);
	}

	void TextureView::bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_ID);
	}

	// very useful -_-
	static GLenum image_format_from_potential_depth_format(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::Depth16: return (GLenum)TextureFormat::R16;
		case TextureFormat::Depth32F: return (GLenum)TextureFormat::R32F;
		default:
			return (GLenum)format;
		}
	}

	void TextureView::bind_as_image(uint32_t slot, TextureAccessMode mode) const
	{
		glBindImageTexture(slot, m_ID, 0, GL_FALSE, 0, (GLenum)mode, image_format_from_potential_depth_format(m_InternalFormat));
	}

	Texture3D::Texture3D(uint32_t width, uint32_t height, uint32_t depth, TextureFormat format, uint32_t mips)
		: m_Width(width), m_Height(height), m_Depth(depth), m_InternalFormat((GLenum)format)
	{
		m_DataFormat = gl_data_format_from_internal_format(format);

		glCreateTextures(GL_TEXTURE_3D, 1, &m_ID);
		glTextureStorage3D(m_ID, mips, (GLenum)format, m_Width, m_Height, m_Depth);
	}

	Texture3D::~Texture3D()
	{
		glDeleteTextures(1, &m_ID);
	}

	void Texture3D::set_filter_mode(TextureFilterMode mode)
	{
		glTextureParameteri(m_ID, GL_TEXTURE_MIN_FILTER, (GLenum)mode);
		glTextureParameteri(m_ID, GL_TEXTURE_MAG_FILTER, (GLenum)mode);
	}

	void Texture3D::set_wrap_mode(TextureWrapMode mode)
	{	
		glTextureParameteri(m_ID, GL_TEXTURE_WRAP_S, (GLenum)mode);
		glTextureParameteri(m_ID, GL_TEXTURE_WRAP_T, (GLenum)mode);
		glTextureParameteri(m_ID, GL_TEXTURE_WRAP_R, (GLenum)mode);
	}

	void Texture3D::set_data(const void* data, uint32_t x, uint32_t y, uint32_t z, uint32_t mip)
	{
		GLenum dataType;
		switch (m_DataFormat)
		{
		case GL_RED: dataType = GL_UNSIGNED_BYTE; break;
		case GL_RG: dataType = GL_UNSIGNED_SHORT; break;
		case GL_RGB: dataType = GL_UNSIGNED_INT; break;
		case GL_RGBA: dataType = GL_UNSIGNED_INT_8_8_8_8; break;
		case GL_RED_INTEGER: dataType = GL_UNSIGNED_BYTE; break;
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureSubImage3D(m_ID, mip, x, y, z, m_Width, m_Height, m_Depth, m_DataFormat, dataType, data);

		if (m_PixelData) delete[] m_PixelData;
		m_PixelData = new uint8_t[m_Width * m_Height * m_Depth];
		memcpy(m_PixelData, data, m_Width * m_Height * m_Depth);
	}

	void Texture3D::generate_mips()
	{
		glGenerateTextureMipmap(m_ID);
	}

	void Texture3D::bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_ID);
	}
	
	void Texture3D::bind_as_image(uint32_t slot, TextureAccessMode mode, uint32_t mip) const
	{
		glBindImageTexture(slot, m_ID, mip, GL_FALSE, 0, (GLenum)mode, m_InternalFormat);
	}

	uint64_t Texture3D::get_bindless_image_handle(uint32_t mip) const
	{
		uint64_t handle = glGetImageHandleARB(m_ID, mip, GL_FALSE, 0, m_InternalFormat);
		return handle;
	}

	owning_ptr<Texture3D> Texture3D::create(uint32_t width, uint32_t height, uint32_t depth, TextureFormat format, uint32_t mips)
	{
		return owning_ptr<Texture3D>(new Texture3D(width, height, depth, format, mips));
	}

	BindlessTexture3D::BindlessTexture3D(const owning_ptr<Texture3D>& texture, uint32_t mip)
	{
		m_Handle = texture->get_bindless_image_handle(mip);
	}

	void BindlessTexture3D::activate(TextureAccessMode mode)
	{
		glMakeImageHandleResidentARB(m_Handle, (GLenum)mode);
	}

	void BindlessTexture3D::deactivate()
	{
		glMakeImageHandleNonResidentARB(m_Handle);
	}
	
}