#pragma once

namespace Engine {

	class Log
	{
	public:
		template<typename T>
		static void print(const T& v)
		{
			std::cout << std::format("{}", v) << "\n";
		}
		template<typename T>
		static void write(const T& v)
		{
			std::cout << std::format("{}", v);
		}

		template<typename... Args>
		static void print(const char* fmt, Args&&... args)
		{
			std::cout << std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)) << "\n";
		}
		template<typename... Args>
		static void write(const char* fmt, Args&&... args)
		{
			std::cout << std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
		}
	};

}