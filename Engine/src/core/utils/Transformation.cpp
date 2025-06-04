#include "pch.h"

namespace Engine {

	Matrix4 Transformation::get_rotation() const
	{
		return
			glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.z), Float3(0, 0, 1)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.y), Float3(0, 1, 0)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.x), Float3(1, 0, 0));
	}

	Matrix4 Transformation::get_transform() const
	{
		return
			glm::translate(glm::mat4(1.0f), Position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.z), Float3(0, 0, 1)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.y), Float3(0, 1, 0)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.x), Float3(1, 0, 0)) *
			glm::scale(glm::mat4(1.0f), Scale);
	}

}