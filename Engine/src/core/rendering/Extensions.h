#pragma once

namespace Engine {

	struct ExtensionsQuery
	{
		bool bindless_texture = false;
		bool sparse_texture = false;
	};

	ExtensionsQuery graphics_query_gl_extension_support();

}