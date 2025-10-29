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

	owning_ptr<VertexBuffer> VertexBuffer::create(void* data, uint32_t size)
	{
		return owning_ptr<VertexBuffer>(new VertexBuffer(data, size));
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

	owning_ptr<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t count)
	{
		return owning_ptr<IndexBuffer>(new IndexBuffer(indices, count));
	}

	ShaderStorageBuffer::ShaderStorageBuffer(const void* data, size_t size)
	{
		glCreateBuffers(1, &m_ID);
		glNamedBufferData(m_ID, size, data, GL_DYNAMIC_DRAW);
	}

	ShaderStorageBuffer::~ShaderStorageBuffer()
	{
		glDeleteBuffers(1, &m_ID);
	}

	void ShaderStorageBuffer::bind(uint32_t slot)
	{
		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, m_ID);
	}

	void ShaderStorageBuffer::set_data(const void* data, size_t offset, size_t size)
	{
		glNamedBufferSubData(m_ID, offset, size, data);
	}

}