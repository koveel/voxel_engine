#include "pch.h"

#include "VertexArray.h"

#include <glad/glad.h>

namespace Engine {

	VertexArray::VertexArray()
	{
		glCreateVertexArrays(1, &m_ID);
	}

	VertexArray::~VertexArray()
	{
		glDeleteVertexArrays(1, &m_ID);
	}

	static uint32_t shader_data_type_to_opengl_type(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:    return GL_FLOAT;
			case ShaderDataType::Float2:   return GL_FLOAT;
			case ShaderDataType::Float3:   return GL_FLOAT;
			case ShaderDataType::Float4:   return GL_FLOAT;
			case ShaderDataType::Mat3:     return GL_FLOAT;
			case ShaderDataType::Mat4:     return GL_FLOAT;
			case ShaderDataType::Int:      return GL_INT;
			case ShaderDataType::Int2:     return GL_INT;
			case ShaderDataType::Int3:     return GL_INT;
			case ShaderDataType::Int4:     return GL_INT;
			case ShaderDataType::Bool:     return GL_BOOL;
		}

		ASSERT(false);
	}

	static uint32_t get_data_type_component_count(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:   return 1;
		case ShaderDataType::Float2:  return 2;
		case ShaderDataType::Float3:  return 3;
		case ShaderDataType::Float4:  return 4;
		case ShaderDataType::Mat3:    return 3; // 3* float3
		case ShaderDataType::Mat4:    return 4; // 4* float4
		case ShaderDataType::Int:     return 1;
		case ShaderDataType::Int2:    return 2;
		case ShaderDataType::Int3:    return 3;
		case ShaderDataType::Int4:    return 4;
		case ShaderDataType::Bool:    return 1;
		}

		ASSERT(false);
	}

	void VertexArray::add_vertex_buffer(owning_ptr<VertexBuffer> vbo)
	{
		glBindVertexArray(m_ID);

		auto& layout = vbo->get_layout();
		for (BufferLayoutElement& element : layout.m_Elements)
		{
			glEnableVertexArrayAttrib(m_ID, m_VBOIndex);
			glVertexArrayVertexBuffer(m_ID, m_VBOIndex, vbo->get_handle(), element.offset, layout.m_Stride);
			glVertexArrayAttribFormat(m_ID, m_VBOIndex, get_data_type_component_count(element.dataType), shader_data_type_to_opengl_type(element.dataType), element.normalized, 0);
			m_VBOIndex++;
		}

		m_VertexBuffers.push_back(std::move(vbo));
	}

	void VertexArray::set_index_buffer(owning_ptr<IndexBuffer> ibo)
	{
		glVertexArrayElementBuffer(m_ID, ibo->get_handle());
		m_IndexBuffer = std::move(ibo);
	}

	void VertexArray::bind() const
	{
		glBindVertexArray(m_ID);
	}

	owning_ptr<VertexArray> VertexArray::create()
	{
		return owning_ptr<VertexArray>(new VertexArray());
	}

}