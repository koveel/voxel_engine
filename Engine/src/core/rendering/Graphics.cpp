#include "pch.h"

#include <glad/glad.h>
#include "Graphics.h"

#include "Shader.h"
#include "VertexArray.h"

#include "gui/Font.h"

#include "gui/MSDFData.h"

namespace Engine {

	void opengl_message_callback(
		unsigned int source, unsigned int type, unsigned int id, unsigned int severity,
		int length, const char* message, const void* userParam)
	{
		LOG(source);
		
		switch (severity)
		{
			case GL_DEBUG_SEVERITY_HIGH:         LOG(message); return;
			case GL_DEBUG_SEVERITY_MEDIUM:       LOG(message); return;
			case GL_DEBUG_SEVERITY_LOW:          LOG(message); return;
			case GL_DEBUG_SEVERITY_NOTIFICATION: LOG(message); return;
		}

		ASSERT(false);
	}
	
	static owning_ptr<VertexArray> s_QuadVAO;
	static owning_ptr<VertexArray> s_CubeVAO;
	static owning_ptr<VertexArray> s_SphereVAO;
	static owning_ptr<VertexArray> s_FullscreenTriangleVAO;

	static void init_fullscreen_triangle()
	{
		s_FullscreenTriangleVAO = VertexArray::create();

		float vertices[] =
		{
			-1.0f, -3.0f,
			 3.0f,  1.0f,
			-1.0f,  1.0f,
		};

		auto vbo = VertexBuffer::create(vertices, sizeof(vertices));
		vbo->set_layout({
			{ ShaderDataType::Float2 },
		});
		s_FullscreenTriangleVAO->add_vertex_buffer(std::move(vbo));
	}

	extern void init_text();

	static void init_quad()
	{
		s_QuadVAO = VertexArray::create();

		Float4 vertices[] =
		{
			{  0.5f,  0.5f, 1.0f, 1.0f },
			{ -0.5f,  0.5f, 0.0f, 1.0f },
			{ -0.5f, -0.5f, 0.0f, 0.0f },
			{  0.5f, -0.5f, 1.0f, 0.0f },
		};

		uint32_t indices[] =
		{
			0, 1, 2, 2, 3, 0
		};

		auto vbo = VertexBuffer::create(vertices, sizeof(vertices));
		vbo->set_layout({
			{ ShaderDataType::Float2 },
			{ ShaderDataType::Float2 },
		});

		s_QuadVAO->add_vertex_buffer(std::move(vbo));
		s_QuadVAO->set_index_buffer(IndexBuffer::create(indices, std::size(indices)));
	}

	static void init_cube()
	{
		s_CubeVAO = VertexArray::create();

		Float3 vertices[] =
		{
			{ -0.5f, -0.5f, -0.5f },
			{  0.5f, -0.5f, -0.5f },
			{  0.5f,  0.5f, -0.5f },
			{ -0.5f,  0.5f, -0.5f },
			{ -0.5f, -0.5f,  0.5f },
			{  0.5f, -0.5f,  0.5f },
			{  0.5f,  0.5f,  0.5f },
			{ -0.5f,  0.5f,  0.5f }
		};

		uint32_t indices[] =
		{
			0, 1, 3, 3, 1, 2,
			1, 5, 2, 2, 5, 6,
			5, 4, 6, 6, 4, 7,
			4, 0, 7, 7, 0, 3,
			3, 2, 7, 7, 2, 6,
			4, 5, 0, 0, 5, 1
		};

		auto vbo = VertexBuffer::create(vertices, sizeof(vertices));
		vbo->set_layout({
			{ ShaderDataType::Float3 },
		});

		s_CubeVAO->add_vertex_buffer(std::move(vbo));
		s_CubeVAO->set_index_buffer(IndexBuffer::create(indices, std::size(indices)));
	}

	static uint32_t s_SphereIndicesTest;	
	static void init_sphere()
	{
		s_SphereVAO = VertexArray::create();
		
		std::vector<Float3> vertices;
		std::vector<uint32_t> indices;

		float radius = 0.5f;
		uint32_t sectorCount = 32;
		uint32_t stackCount = 64;

		float sectorStep = 2 * glm::pi<float>() / sectorCount;
		float stackStep = glm::pi<float>() / stackCount;
		float sectorAngle, stackAngle;

		for (int i = 0; i <= stackCount; ++i)
		{
			stackAngle = glm::pi<float>() / 2 - i * stackStep;
			float xy = radius * cosf(stackAngle);
			float z = radius * sinf(stackAngle);

			for (int j = 0; j <= sectorCount; ++j)
			{
				sectorAngle = j * sectorStep;

				float x = xy * cosf(sectorAngle);
				float y = xy * sinf(sectorAngle);

				vertices.push_back({ x, y, z });
			}
		}

		for (int i = 0; i < stackCount; ++i)
		{
			int k1 = i * (sectorCount + 1);
			int k2 = k1 + sectorCount + 1;

			for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
			{
				if (i != 0)
				{
					indices.push_back(k1);
					indices.push_back(k2);
					indices.push_back(k1 + 1);
				}

				if (i != (stackCount - 1))
				{
					indices.push_back(k1 + 1);
					indices.push_back(k2);
					indices.push_back(k2 + 1);
				}
			}
		}

		auto vbo = VertexBuffer::create(vertices.data(), vertices.size() * sizeof(Float3));
		vbo->set_layout({
			{ ShaderDataType::Float3 },
		});

		s_SphereIndicesTest = indices.size();
		s_SphereVAO->add_vertex_buffer(std::move(vbo));
		s_SphereVAO->set_index_buffer(IndexBuffer::create(indices.data(), indices.size()));
	}

	void Graphics::init()
	{
#ifdef ENGINE_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(opengl_message_callback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);
#endif
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		set_depth_test(DepthTest::Less);
		glClearDepth(1.0f);
		glLineWidth(2);

		set_face_cull(Face::Back);
		init_quad();
		init_cube();
		init_text();
		init_sphere();
		init_fullscreen_triangle();
	}

	void Graphics::set_color_mask(uint32_t buf, bool r, bool g, bool b, bool a)
	{
		glColorMaski(buf, r, g, b, a);
	}

	void Graphics::set_face_cull(Face face)
	{
		if (face == Face::None) {
			glDisable(GL_CULL_FACE);
			return;
		}

		glEnable(GL_CULL_FACE);
		glCullFace((GLenum)face);
	}

	void Graphics::toggle_blend(bool enabled)
	{
		if (enabled)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);
	}

	void Graphics::reset_draw_buffers()
	{
		glDrawBuffer(GL_NONE);
	}

	void Graphics::set_draw_buffers(uint32_t* buffers, size_t count)
	{
		glDrawBuffers(count, buffers);
	}

	void Graphics::set_blend_function(uint32_t sfactor, uint32_t dfactor)
	{
		glBlendFunc(sfactor, dfactor);
	}

	static PrimitiveMode s_PrimitiveDrawMode = PrimitiveMode::Triangles;

	void Graphics::set_draw_mode(PrimitiveMode mode)
	{
		s_PrimitiveDrawMode = mode;
	}

	void Graphics::memory_barrier(uint32_t target)
	{
		glMemoryBarrier(target);
	}

	void Graphics::set_depth_mask(bool mask)
	{
		glDepthMask(mask);
	}

	void Graphics::set_depth_test(DepthTest mode)
	{
		if (mode == DepthTest::Off) {
			glDisable(GL_DEPTH_TEST);
			return;
		}

		glEnable(GL_DEPTH_TEST);
		glDepthFunc((GLenum)mode);
	}

	void Graphics::set_stencil_test(bool enabled)
	{
		if (enabled)
			glEnable(GL_STENCIL_TEST);
		else
			glDisable(GL_STENCIL_TEST);
	}

	void Graphics::set_stencil_func(StencilTest func, uint8_t ref, uint8_t mask)
	{
		glStencilFunc((GLenum)func, ref, mask);
	}

	void Graphics::set_face_stencil_op(Face face, StencilOp stencilFail, StencilOp stencilPassDepthFail, StencilOp stencilDepthPass)
	{
		glStencilOpSeparate((GLenum)face, (GLenum)stencilFail, (GLenum)stencilPassDepthFail, (GLenum)stencilDepthPass);
	}

	void Graphics::set_stencil_mask(uint8_t mask)
	{
		glStencilMask(mask);
	}

	void Graphics::clear(const Color& color, float depth)
	{
		glClearDepth(depth);
		glClearColor(color.r, color.g, color.b, color.a);
		glColorMask(1, 1, 1, 1); // TODO: save state prob
		glDepthMask(true);		 // TODO: save state prob

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	void Graphics::draw_indexed(uint32_t count)
	{
		glDrawElements((GLenum)s_PrimitiveDrawMode, count, GL_UNSIGNED_INT, nullptr);
	}

	static Float2 s_ViewportDims;

	void Graphics::draw_cube()
	{
		s_CubeVAO->bind();
		draw_indexed(36);
	}

	void Graphics::draw_sphere()
	{
		s_SphereVAO->bind();
		draw_indexed(s_SphereIndicesTest);
	}

	void Graphics::draw_quad()
	{
		s_QuadVAO->bind();
		draw_indexed(6);
	}

	void Graphics::draw_fullscreen_triangle(const owning_ptr<Shader>& shader)
	{
		shader->bind();
		s_FullscreenTriangleVAO->bind();

		shader->set_float2("u_ViewportDims", s_ViewportDims);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}	

	void Graphics::resize_viewport(uint32_t x, uint32_t y)
	{
		s_ViewportDims = { (float)x, (float)y };
		glViewport(0, 0, x, y);
	}

}