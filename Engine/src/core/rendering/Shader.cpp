#include "pch.h"

#include <fstream>
#include "Shader.h"

#include <glad/glad.h>

namespace Engine {

	Shader::Shader()
	{
	}

	Shader::~Shader()
	{
		glDeleteProgram(m_ID);
	}

	void Shader::bind() const
	{
		glUseProgram(m_ID);
	}

	int Shader::get_or_cache_uniform_location(const char* name)
	{
		int location = -1;
		if (!m_UniformLocations.count(name))
			m_UniformLocations[name] = glGetUniformLocation(m_ID, name);

		location = m_UniformLocations[name];
		//if (location == -1)
		//	LOG("couldn't get uniform '{}' location in shader", name);

		return location;
	}

	void Shader::set_int(const std::string& name, int value)
	{
		int location = get_or_cache_uniform_location(name.c_str());
		glProgramUniform1i(m_ID, location, value);
	}

	void Shader::set_int2(const std::string& name, const Int2& v)
	{
		int location = get_or_cache_uniform_location(name.c_str());
		glProgramUniform2i(m_ID, location, v.x, v.y);
	}

	void Shader::set_int3(const std::string& name, const Int3& v)
	{
		int location = get_or_cache_uniform_location(name.c_str());
		glProgramUniform3i(m_ID, location, v.x, v.y, v.z);
	}

	void Shader::set_int4(const std::string& name, const Int4& v)
	{
		int location = get_or_cache_uniform_location(name.c_str());
		glProgramUniform4i(m_ID, location, v.x, v.y, v.z, v.w);
	}

	//void Shader::set_int_array(const std::string& name, int* values, uint32_t count)
	//{
	//	int location = get_or_cache_uniform_location(name.c_str());
	//	glProgramUniform1iv(m_ID, location, count, values);
	//}

	void Shader::set_float(const std::string& name, float value)
	{
		int location = get_or_cache_uniform_location(name.c_str());
		glProgramUniform1f(m_ID, location, value);
	}

	void Shader::set_float2(const std::string& name, const Float2& v)
	{
		int location = get_or_cache_uniform_location(name.c_str());
		glProgramUniform2f(m_ID, location, v.x, v.y);
	}

	void Shader::set_float3(const std::string& name, const Float3& v)
	{
		int location = get_or_cache_uniform_location(name.c_str());
		glProgramUniform3f(m_ID, location, v.x, v.y, v.z);
	}

	void Shader::set_float4(const std::string& name, const Float4& v)
	{
		int location = get_or_cache_uniform_location(name.c_str());
		glProgramUniform4f(m_ID, location, v.x, v.y, v.z, v.w);
	}

	void Shader::set_matrix(const std::string& name, const Matrix4& v)
	{
		int location = get_or_cache_uniform_location(name.c_str());
		glProgramUniformMatrix4fv(m_ID, location, 1, false, glm::value_ptr(v));
	}

	static std::string read_file(const std::filesystem::path& filepath)
	{
		std::string contents;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			size_t size = in.tellg();
			if (size != -1)
			{
				contents.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&contents[0], size);
				in.close();
			}
			else
			{
				LOG("could not read shader file '{}'", filepath.string());
			}
		}
		else
		{
			LOG("could not open shader file '{}'", filepath.string());
		}

		return contents;
	}

	// kind of scuffed preprocessing system - but does the job for now
	// mostly to support "#include"s and stuff in shaders organization
	static std::array<std::string, 2> preprocess_shader_string(const std::string& source, const std::filesystem::path& directory)
	{
		std::array<std::string, 2> result; // vertex, fragment
		result[0].reserve(source.length());
		result[1].reserve(source.length());

		const char PreprocessorToken = '#';

		size_t currentShaderType = 0; // vertex
		for (size_t i = 0; i < source.length(); i++)
		{
			char current = source[i];

			// append char
			if (current != PreprocessorToken || source[i + 1] == 'v') { // need to not remove '#' in #version
				result[currentShaderType] += current;
				continue;
			}

			if (strncmp(&source[i], "#type ", 6) == 0)
			{
				const char* type = &source[i + 6];
				if (strncmp(type, "vertex", 6) == 0) {
					currentShaderType = 0;
					i += 12;
				}
				else if (strncmp(type, "fragment", 8) == 0) {
					currentShaderType = 1;
					i += 14;
				}
			}

			if (strncmp(&source[i], "#include ", 9) == 0)
			{
				const char* file = &source[i + 10];
				std::string fileName = std::string(std::string_view(file, find_nth_index_of_char(file, 0, '"')));

				auto filePath = directory / fileName;
				std::string fileSource = read_file(filePath.string());
				result[currentShaderType] += fileSource;

				i += 10 + fileName.length();
			}
		}

		result[0].shrink_to_fit();
		result[1].shrink_to_fit();
		return { result[0], result[1] };
	}

	static const char* shader_type_to_string(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::Vertex: return "vertex";
		case ShaderType::Fragment: return "fragment";
		case ShaderType::Compute: return "compute";
		}
	}

	static uint32_t create_and_attach_shader_to_program(uint32_t program, ShaderType type, const std::string& source)
	{
		uint32_t shader = glCreateShader((GLenum)type);

		const char* src = source.c_str();
		glShaderSource(shader, 1, &src, 0);
		glCompileShader(shader);

		int isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

		if (!isCompiled)
		{
			int maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<char> infoLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

			glDeleteShader(shader);

			// this shit sucks
			//long possibleLine = 0;
			//if (possibleLine = strtol(&infoLog[2], nullptr, 10))
			//{
			//	possibleLine += std::count(source, source, '\n');
			//	LOG("[possible {} shader actual line: {}]", shader_type_to_string(type), possibleLine);
			//}

			LOG(infoLog.data());
			ASSERT(false && "failed to compile shader");
		}

		glAttachShader(program, shader);
		return shader;
	}

	owning_ptr<Shader> Shader::create(const std::filesystem::path& filepath)
	{
		auto result = owning_ptr<Shader>(new Shader());

		constexpr uint32_t ShaderTypeCount = 2;
		std::string fileContents = read_file(filepath);

		std::array<std::string, 2> shaderSources = preprocess_shader_string(fileContents, filepath.parent_path());
		bool noFragmentShader = shaderSources[1].empty();

		uint32_t program = glCreateProgram();
		std::vector<uint32_t> shaderIDs;
		shaderIDs.reserve(ShaderTypeCount);

		for (int i = 0; i < ShaderTypeCount - noFragmentShader; i++)
		{
			uint32_t shader = create_and_attach_shader_to_program(program, i == 0 ? ShaderType::Vertex : ShaderType::Fragment, shaderSources[i]);
			shaderIDs.push_back(shader);
		}

		result->m_ID = program;
		glLinkProgram(program);

		int isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (!isLinked)
		{
			int maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<char> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

			glDeleteProgram(program);
			
			for (auto id : shaderIDs)
				glDeleteShader(id);

			LOG(infoLog.data());
			ASSERT(false && "failed to link shader");
			
			return nullptr;
		}

		for (uint32_t id : shaderIDs)
		{
			glDetachShader(program, id);
			glDeleteShader(id);
		}

		return result;
	}

	ComputeShader::ComputeShader()
	{
	}

	ComputeShader::~ComputeShader()
	{
		glDeleteProgram(m_ID);
	}

	void ComputeShader::bind() const
	{
		glUseProgram(m_ID);
		//glBindProgramPipeline(m_ID);
	}

	void ComputeShader::dispatch(uint32_t x, uint32_t y, uint32_t z) const
	{
		glUseProgram(m_ID);
		glDispatchCompute(x, y, z);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	owning_ptr<ComputeShader> ComputeShader::create(const std::filesystem::path& filepath)
	{
		auto compute = owning_ptr<ComputeShader>(new ComputeShader());

		constexpr uint32_t ShaderTypeCount = 2;
		std::string fileContents = read_file(filepath);

		uint32_t program = glCreateProgram();
		uint32_t shader = create_and_attach_shader_to_program(program, ShaderType::Compute, fileContents);

		compute->m_ID = program;
		glLinkProgram(program);

		int isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (!isLinked)
		{
			int maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<char> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

			glDeleteProgram(program);
			glDeleteShader(shader);

			LOG(infoLog.data());
			ASSERT(false && "failed to link shader");

			return nullptr;
		}

		glDetachShader(program, shader);
		glDeleteShader(shader);

		return compute;
	}

}