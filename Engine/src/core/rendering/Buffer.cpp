#include "pch.h"
#include "Buffer.h"

#include <glad/glad.h>

namespace Engine {

	VertexBuffer::VertexBuffer(void* vertices, uint32_t size)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, size, vertices, vertices ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	}

	VertexBuffer::~VertexBuffer()
	{
		glDeleteBuffers(1, &m_ID);
	}

	void VertexBuffer::set_data(void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(m_ID, offset, size, data);
	}

	std::unique_ptr<VertexBuffer> VertexBuffer::create(void* data, uint32_t size)
	{
		return std::unique_ptr<VertexBuffer>(new VertexBuffer(data, size));
	}

	IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t count)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
	}

	IndexBuffer::~IndexBuffer()
	{
		glDeleteBuffers(1, &m_ID);
	}

	std::unique_ptr<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t count)
	{
		return std::unique_ptr<IndexBuffer>(new IndexBuffer(indices, count));
	}

}