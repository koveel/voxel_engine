#pragma once

#define LOG(x, ...) Log::print(x, __VA_ARGS__)

// todo: runtime asserts idk
#ifndef ENGINE_RELEASE
	#define ASSERT(x) { if (!(x)) { LOG(##x); __debugbreak(); } }
#else
	#define ASSERT(x) { if (!(x)) { LOG(##x); } }
#endif

#define DELEGATE(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }