#pragma once

#include "Buffer.h"

namespace Engine {

	class VertexArray
	{
	private:
		VertexArray();
	public:
		~VertexArray();

		void bind() const;

		void add_vertex_buffer(std::unique_ptr<VertexBuffer> vbo);
		void set_index_buffer(std::unique_ptr<IndexBuffer> ibo);

		uint32_t get_handle() const { return m_ID; }

		static std::unique_ptr<VertexArray> create();
	private:
		uint32_t m_ID = 0;

		uint32_t m_VBOIndex = 0;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::vector<std::unique_ptr<VertexBuffer>> m_VertexBuffers;
	};

}