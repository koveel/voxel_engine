#pragma once

namespace Engine {

	class Shader
	{
	private:
		Shader();
	public:
		~Shader();

		void bind() const;

		void set_int(const std::string& name, int value);
		void set_int2(const std::string& name, const Int2& v);
		void set_int3(const std::string& name, const Int3& v);
		void set_int4(const std::string& name, const Int4& v);
		void set_float(const std::string& name, float value);
		void set_float2(const std::string& name, const Float2& v);
		void set_float3(const std::string& name, const Float3& v);
		void set_float4(const std::string& name, const Float4& v);
		void set_matrix(const std::string& name, const Matrix4& v);

		void set_int_array(const std::string& name, int* values, uint32_t count);

		int get_or_cache_uniform_location(const char* name);

		static std::unique_ptr<Shader> create(const std::filesystem::path& filepath);
	private:
		uint32_t m_ID = 0;
		std::unordered_map<std::string, int> m_UniformLocations;
	};

}