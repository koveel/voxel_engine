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