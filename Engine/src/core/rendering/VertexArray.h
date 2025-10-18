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

		void add_vertex_buffer(owning_ptr<VertexBuffer> vbo);
		void set_index_buffer(owning_ptr<IndexBuffer> ibo);

		uint32_t get_handle() const { return m_ID; }

		static owning_ptr<VertexArray> create();
	private:
		uint32_t m_ID = 0;

		uint32_t m_VBOIndex = 0;
		owning_ptr<IndexBuffer> m_IndexBuffer;
		std::vector<owning_ptr<VertexBuffer>> m_VertexBuffers;
	};

}