#pragma once

#include "Texture.h"

namespace Engine {

	struct FramebufferDescriptor
	{
		struct ColorAttachment
		{
			uint32_t Width = 0, Height = 0;
			TextureFormat Format = TextureFormat::RGBA8;
			bool Clear = true;
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

		void bind();
		void clear(const Color& color);
		void resize(uint32_t width, uint32_t height);

		void set_index_handle(uint32_t index, uint32_t handle);

		uint32_t get_handle() const { return m_ID; }

		static void unbind();
		static owning_ptr<Framebuffer> create(const FramebufferDescriptor& descriptor);
	private:
		uint32_t m_ID = 0;
	public:
		uint32_t m_DrawBuffers[8]{};
		uint32_t m_DrawBuffersCount = 0;

		uint32_t m_ClearBuffers[8]{};
		uint32_t m_ClearBuffersCount = 0;

		std::vector<owning_ptr<Texture2D>> m_ColorAttachments;
		owning_ptr<Texture2D> m_DepthStencilAttachment;
	};

}