#include "pch.h"

#include "Extensions.h"

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Engine {

	static Int3 get_format_sparse_virtual_page_size_3d(GLenum format)
	{
		int x, y, z;
		glGetInternalformativ(GL_TEXTURE_3D, format, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &x);
		glGetInternalformativ(GL_TEXTURE_3D, format, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &y);
		glGetInternalformativ(GL_TEXTURE_3D, format, GL_VIRTUAL_PAGE_SIZE_Z_ARB, 1, &z);
		return { x, y, z };
	}

	ExtensionsQuery graphics_query_gl_extension_support()
	{
		ExtensionsQuery query;
		query.bindless_texture = glfwExtensionSupported("GL_ARB_bindless_texture");
		query.sparse_texture = glfwExtensionSupported("GL_ARB_sparse_texture");

		LOG("extension query:");

		LOG("GL_ARB_bindless_texture:\n\t{}", query.bindless_texture ? "YES" : "NO");

		LOG("GL_ARB_sparse_texture:\n\t{}",   query.sparse_texture   ? "YES" : "NO");
		{
			int max_sparse_size, max_sparse_size3d;
			glGetIntegerv(GL_MAX_SPARSE_TEXTURE_SIZE_ARB, &max_sparse_size);
			glGetIntegerv(GL_MAX_SPARSE_3D_TEXTURE_SIZE_ARB, &max_sparse_size3d);
			LOG("\tMax sparse size 2D: {}", max_sparse_size);
			LOG("\tMax sparse size 3D: {}", max_sparse_size3d);

			Int3 r8ui = get_format_sparse_virtual_page_size_3d(GL_R8UI);
			LOG("\tR8UI vpage size = [{}, {}, {}]", r8ui.x, r8ui.y, r8ui.z);
			Int3 rgba8 = get_format_sparse_virtual_page_size_3d(GL_RGBA8);
			LOG("\tRGBA8 vpage size = [{}, {}, {}]", rgba8.x, rgba8.y, rgba8.z);
		}
		LOG("");

		return query;
	}

}