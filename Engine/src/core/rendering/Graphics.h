#pragma once

namespace Engine {

	enum class Face
	{
		None  = 0,
		Front = 0x0404,
		Back  = 0x0405,
		FrontAndBack = 0x0408,
	};

	enum class Comparison
	{
		Off       = 0,
		Less      = 0x0201,
		LessEq    = 0x0203,
		Equal     = 0x0202,
		NotEqual  = 0X0205,
		Greater   = 0x0204,
		GreaterEq = 0x0206,
		Always    = 0x0207,
	};
	using DepthTest = Comparison;
	using StencilTest = Comparison;

	enum class StencilOp
	{
		Keep          = 0x1E00,
		Replace       = 0x1E01,
		Increment     = 0X1E02,
		IncrementWrap = 0X8507,
		Decrement     = 0X1E03,
		DecrementWrap = 0X8508,
	};

	enum class PrimitiveMode
	{
		LineStrip = 0x0003,
		Triangles = 0x0004,
	};

	class Graphics
	{
	public:
		static void init();

		static void clear(const Color& color, float depth = 1.0f);
		static void draw_indexed(uint32_t count);

		static void set_color_mask(uint32_t buf, bool r, bool g, bool b, bool a);
		static void set_depth_mask(bool mask);
		static void set_stencil_mask(uint8_t mask);

		static void set_face_cull(Face faces);
		static void set_depth_test(DepthTest mode);

		static void set_stencil_test(bool enabled);
		static void set_stencil_func(StencilTest func, uint8_t ref, uint8_t mask);
		static void set_face_stencil_op(Face face, StencilOp stencilFail, StencilOp stencilPassDepthFail, StencilOp stencilDepthPass);

		static void toggle_blend(bool enabled);

		static void reset_draw_buffers(); // set to GL_NONE
		static void set_draw_buffers(uint32_t* buffers, size_t count);

		static void set_blend_function(uint32_t sfactor, uint32_t dfactor);
		static void set_draw_mode(PrimitiveMode mode);

		static void memory_barrier(uint32_t target);

		static void draw_cube();
		static void draw_sphere();
		static void draw_quad();
		static void draw_fullscreen_triangle(const owning_ptr<class Shader>& shader);
		static void draw_text(const std::string& text, const owning_ptr<class Font>& font, float tracking = 0.0f);

		static void resize_viewport(uint32_t x, uint32_t y);
	};

}