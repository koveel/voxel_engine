#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Engine {

	using Int2 = glm::ivec2;
	using Int3 = glm::ivec3;
	using Int4 = glm::ivec4;

	using Float2 = glm::vec2;
	using Float3 = glm::vec3;
	using Float4 = glm::vec4;
	using Matrix4 = glm::mat4;

}

namespace std {

	template<>
	struct hash<Engine::Int2> {
		size_t operator()(const Engine::Int2& key) const {
			size_t ux = static_cast<size_t>(key.x >= 0 ? 2 * key.x : -2 * key.x - 1);
			size_t uy = static_cast<size_t>(key.y >= 0 ? 2 * key.y : -2 * key.y - 1);

			// Szudzik or wtv tf
			return ux >= uy ? ux * ux + ux + uy : uy * uy + ux;
		}
	};

}