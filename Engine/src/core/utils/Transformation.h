#pragma once

namespace Engine {

	struct Transformation
	{
		Float3 Position = { 0.0f, 0.0f, 0.0f };
		Float3 Rotation = { 0.0f, 0.0f, 0.0f };
		Float3 Scale = { 1.0f, 1.0f, 1.0f };

		Transformation() = default;
		Transformation(const Float3& position, const Float3& euler = {0.0f, 0.0f, 0.0f}, const Float3& scale = {1.0f, 1.0f, 1.0f})
			: Position(position), Rotation(euler), Scale(scale) {}

		Matrix4 get_rotation() const;
		Matrix4 get_transform() const;
	};

}