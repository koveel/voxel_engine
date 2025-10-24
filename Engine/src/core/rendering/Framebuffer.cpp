#include "pch.h"

#include "Framebuffer.h"

#include <glad/glad.h>
#include "Graphics.h"

namespace Engine {

	static bool depth_format_has_stencil(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::Depth24Stencil8:
		case TextureFormat::Depth32FStencil8:
			return true;
		default:
			return false;
		}
	}

	Framebuffer::Framebuffer(const FramebufferDescriptor& descriptor)
	{
		glCreateFramebuffers(1, &m_ID);

		uint32_t attachmentIndex = 0;
		m_ColorAttachments.reserve(descriptor.ColorAttachments.size());

		for (const FramebufferDescriptor::ColorAttachment& attachment : descriptor.ColorAttachments)
		{
			owning_ptr<Texture2D> texture = Texture2D::create(attachment.Width, attachment.Height, attachment.Format);

			uint32_t glAttachment = GL_COLOR_ATTACHMENT0 + attachmentIndex;
			glNamedFramebufferTexture(m_ID, glAttachment, texture->get_handle(), 0);
			m_ColorAttachments.push_back(std::move(texture));

			m_DrawBuffers[m_DrawBuffersCount++] = glAttachment;
			if (attachment.Clear)
				m_ClearBuffers[m_ClearBuffersCount++] = glAttachment;

			attachmentIndex++;
		}
		if (auto pDepthStencil = descriptor.pDepthStencilAttachment)
		{
			owning_ptr<Texture2D> texture = Texture2D::create(pDepthStencil->Width, pDepthStencil->Height, pDepthStencil->Format);

			bool hasStencil = depth_format_has_stencil(pDepthStencil->Format);
			glNamedFramebufferTexture(m_ID, hasStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, texture->get_handle(), 0);

			m_DepthStencilAttachment = std::move(texture);
		}

		glNamedFramebufferDrawBuffers(m_ID, m_DrawBuffersCount, m_DrawBuffers);
		ASSERT(glCheckNamedFramebufferStatus(m_ID, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	}

	Framebuffer::~Framebuffer()
	{
		glDeleteFramebuffers(1, &m_ID);
	}

	void Framebuffer::bind()
	{
		glNamedFramebufferDrawBuffers(m_ID, m_DrawBuffersCount, m_DrawBuffers);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ID);
	}

	void Framebuffer::clear(const Color& color)
	{
		bind();
		glNamedFramebufferDrawBuffers(m_ID, m_ClearBuffersCount, m_ClearBuffers);
		Graphics::clear(color);
		glNamedFramebufferDrawBuffers(m_ID, m_DrawBuffersCount, m_DrawBuffers);
	}	

	void Framebuffer::resize(uint32_t width, uint32_t height)
	{
		if (!this) // Some events are dispatched before Framebuffer is init'd idfk
			return;

		// Recreate attachments
		for (uint32_t attachment = 0; attachment < m_ColorAttachments.size(); attachment++)
		{
			const owning_ptr<Texture2D>& texture = m_ColorAttachments[attachment];
			m_ColorAttachments[attachment] = Texture2D::create(width, height, texture->get_format());
			glNamedFramebufferTexture(m_ID, GL_COLOR_ATTACHMENT0 + attachment, m_ColorAttachments[attachment]->get_handle(), 0);
		}

		if (m_DepthStencilAttachment)
		{
			TextureFormat format = m_DepthStencilAttachment->get_format();
			bool hasStencil = depth_format_has_stencil(format);

			const owning_ptr<Texture2D>& texture = m_DepthStencilAttachment;
			m_DepthStencilAttachment = Texture2D::create(width, height, format);
			glNamedFramebufferTexture(m_ID, hasStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, m_DepthStencilAttachment->get_handle(), 0);
		}
	}

	void Framebuffer::set_index_handle(uint32_t index, uint32_t handle)
	{
		glNamedFramebufferTexture(m_ID, GL_COLOR_ATTACHMENT0 + index, handle, 0);
	}

	void Framebuffer::unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	owning_ptr<Framebuffer> Framebuffer::create(const FramebufferDescriptor& descriptor)
	{
		return owning_ptr<Framebuffer>(new Framebuffer(descriptor));
	}

}