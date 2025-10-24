#pragma once

namespace Engine {

	enum class ShaderType
	{
		Vertex = 0x8B31,
		Fragment = 0x8B30,
		Compute = 0x91B9,
	};

	class Shader
	{
	protected:
		Shader() = default;
	public:
		~Shader();

		void bind() const;

		template<typename T>
		void set(const std::string& name, const T& v);

		int get_or_cache_uniform_location(const std::string& name);

		static owning_ptr<Shader> create(const std::filesystem::path& filepath);
	protected:
		uint32_t m_ID = 0;
		std::unordered_map<std::string, int> m_UniformLocations;
	};

	class ComputeShader : protected Shader
	{
	private:
		ComputeShader() = default;
	public:
		~ComputeShader();

		using Shader::bind;
		using Shader::set;

		void dispatch(uint32_t x, uint32_t y, uint32_t z = 1) const;

		static owning_ptr<ComputeShader> create(const std::filesystem::path& filepath);
	};

}