#pragma once

#include "Texture.h"

namespace Engine {

	struct FramebufferDescriptor
	{
		struct ColorAttachment
		{
			uint32_t Width = 0, Height = 0;
			TextureFormat Format = TextureFormat::RGBA8;
		};
		struct DepthStencilAttachment
		{
			uint32_t Width = 0, Height = 0;
			uint32_t DepthBits = 24;
			bool Stencil = false;
		};

		std::vector<ColorAttachment> ColorAttachments;
		DepthStencilAttachment* pDepthStencilAttachment = nullptr;
	};

	class Framebuffer
	{
	private:
		Framebuffer(const FramebufferDescriptor& descriptor);
	public:
		~Framebuffer();

		void bind(int state = -1);
		void clear(const Color& color);
		void resize(uint32_t width, uint32_t height);

		static std::unique_ptr<Framebuffer> create(const FramebufferDescriptor& descriptor);
	private:
		uint32_t m_ID = 0;
	public:
		std::vector<std::unique_ptr<Texture2D>> m_ColorAttachments;
		std::unique_ptr<Texture2D> m_DepthStencilAttachment;
	};

}