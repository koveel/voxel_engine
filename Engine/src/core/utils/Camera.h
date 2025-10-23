#pragma once

namespace Engine {

	enum class ProjectionType
	{
		Orthographic,
		Perspective,
	};

	class Camera
	{
	public:
		Camera() = default;
		Camera(ProjectionType type);
		Camera(float aspectRatio);

		void set_projection_type(ProjectionType type);
		void set_near(float near);
		void set_far(float far);
		void set_fov(float fov);
		void set_ortho_size(float orthoSize);
		void set_aspect_ratio(float aspectRatio);

		ProjectionType get_projection_type() const { return m_Type; }
		float get_near() const { return m_Near; }
		float get_far() const { return m_Far; }
		float get_fov() const { return m_FOV; }
		float get_ortho_size() const { return m_OrthoSize; }
		float get_aspect_ratio() const { return m_AspectRatio; }

		const Matrix4& get_projection() const { return m_Projection; }
	private:
		void recalculate_projection();
	private:
		ProjectionType m_Type;

		float m_FOV = 60.0f;
		float m_OrthoSize = 10.0f;
		float m_AspectRatio = 1920 / 1080.0f;
		float m_Near = 0.1f;
		float m_Far = 500.0f;
		Matrix4 m_Projection;
	};

}