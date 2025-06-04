#pragma once

namespace Engine {

	struct Color
	{
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 0.0f;

		Color() = default;
		Color(float scalar)
			: r(scalar), g(scalar), b(scalar)
		{}
		Color(float r, float g, float b)
			: r(r), g(g), b(b), a(1.0f)
		{}
		Color(float r, float g, float b, float a)
			: r(r), g(g), b(b), a(a)
		{}
		Color(const Color& other)
			: r(other.r), g(other.g), b(other.b), a(other.a)
		{}

		float* operator&()
		{
			return &r;
		}
	};

}