#include "pch.h"
#include "Camera.h"

namespace Engine {

	Camera::Camera(ProjectionType type)
	{
		set_projection_type(type);
	}

	Camera::Camera(float aspectRatio)
		: m_AspectRatio(aspectRatio)
	{
		recalculate_projection();
	}

	void Camera::set_projection_type(ProjectionType type)
	{
		m_Type = type;
		recalculate_projection();
	}

	void Camera::set_near(float near)
	{
		m_Near = near;
		recalculate_projection();
	}

	void Camera::set_far(float far)
	{
		m_Far = far;
		recalculate_projection();
	}

	void Camera::set_fov(float fov)
	{
		m_FOV = fov;

		if (m_Type == ProjectionType::Perspective)
			recalculate_projection();
	}

	void Camera::set_ortho_size(float orthoSize)
	{
		m_OrthoSize = orthoSize;

		if (m_Type == ProjectionType::Orthographic)
			recalculate_projection();
	}

	void Camera::set_aspect_ratio(float aspectRatio)
	{
		m_AspectRatio = aspectRatio;
		recalculate_projection();
	}

	void Camera::recalculate_projection()
	{
		if (m_Type == ProjectionType::Orthographic)
			m_Projection = glm::ortho(-m_OrthoSize * m_AspectRatio * 0.5f, m_OrthoSize * m_AspectRatio * 0.5f, -m_OrthoSize * 0.5f, m_OrthoSize * 0.5f, m_Near, m_Far);

		if (m_Type == ProjectionType::Perspective)
			m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_Near, m_Far);
	}

}