#pragma once

namespace Engine {

	enum class ShaderDataType
	{
		None = 0,
		Float, Float2, Float3, Float4,
		Mat3, Mat4,
		Int, Int2, Int3, Int4,
		Bool
	};

	static uint32_t get_shader_data_type_size(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:    return 4;
			case ShaderDataType::Float2:   return 4 * 2;
			case ShaderDataType::Float3:   return 4 * 3;
			case ShaderDataType::Float4:   return 4 * 4;
			case ShaderDataType::Mat3:     return 4 * 3 * 3;
			case ShaderDataType::Mat4:     return 4 * 4 * 4;
			case ShaderDataType::Int:      return 4;
			case ShaderDataType::Int2:     return 4 * 2;
			case ShaderDataType::Int3:     return 4 * 3;
			case ShaderDataType::Int4:     return 4 * 4;
			case ShaderDataType::Bool:     return 1;
		}

		ASSERT(false);
	}

	struct BufferLayoutElement
	{
		ShaderDataType dataType;
		uint32_t size;
		size_t offset;
		bool normalized;

		BufferLayoutElement() = default;
		BufferLayoutElement(ShaderDataType type, bool normalized = false)
			: dataType(type), size(get_shader_data_type_size(type)), offset(0), normalized(normalized)
		{
		}		
	};

	class VertexBufferLayout
	{
	public:
		VertexBufferLayout() = default;
		VertexBufferLayout(std::initializer_list<BufferLayoutElement> elements)
			: m_Elements(elements)
		{
			size_t offset = 0;
			m_Stride = 0;
			for (auto& element : m_Elements)
			{
				element.offset = offset;
				offset += element.size;
			}
			m_Stride = offset;
		}
	public:
		std::vector<BufferLayoutElement> m_Elements;
		uint32_t m_Stride = 0;
	};

	class VertexBuffer
	{
	private:
		VertexBuffer(void* vertices, uint32_t size);
	public:
		~VertexBuffer();
		
		VertexBufferLayout& get_layout() { return m_Layout; }
		void set_layout(const VertexBufferLayout& layout) { m_Layout = layout; }
		void set_data(void* data, uint32_t size, uint32_t offset = 0);

		uint32_t get_handle() const { return m_ID; }

		static owning_ptr<VertexBuffer> create(void* data, uint32_t size);
	private:
		uint32_t m_ID = 0;
		VertexBufferLayout m_Layout;
	};

	class IndexBuffer
	{
	private:
		IndexBuffer(uint32_t* indices, uint32_t count);
	public:
		~IndexBuffer();

		uint32_t get_handle() const { return m_ID; }

		static owning_ptr<IndexBuffer> create(uint32_t* indices, uint32_t count);
	private:
		uint32_t m_ID = 0;
	};

	class ShaderStorageBuffer
	{
	private:
		ShaderStorageBuffer(const void* data, size_t size);
	public:
		~ShaderStorageBuffer();

		void bind(uint32_t slot = 0);
		void set_data(const void* data, size_t size);

		uint32_t get_handle() const { return m_ID; }

		static owning_ptr<ShaderStorageBuffer> create(const void* data, size_t size);
	private:
		uint32_t m_ID = 0;
	};

}