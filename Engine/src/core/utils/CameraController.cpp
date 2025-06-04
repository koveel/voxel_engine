#include "pch.h"

#include "CameraController.h"

namespace Engine {

	void CameraController::smooth(float ts)
	{
		Float3& euler = m_Transformation.Rotation;
		Float3& position = m_Transformation.Position;

		float lookT = 25.0f * ts;
		float positionT = 25.0f * ts;

		euler = glm::mix(euler, m_TargetEuler, lookT);
		position = glm::mix(position, m_TargetPosition, positionT);
	}

	void CameraController::update(float ts)
	{
		smooth(ts);

		if (Input::was_key_pressed(Key::Escape)) {
			Input::set_cursor_mode(CursorMode::Default);
		}
		else if (Input::was_key_pressed(Key::LeftMouse)) {
			Input::set_cursor_mode(CursorMode::Locked);
		}
		if (Input::get_cursor_mode() == CursorMode::Default)
			return; // no input		

		float sensitivity = 4.0f * ts;
		float moveSpeed = m_MoveSpeed * ts;

		m_TargetEuler.y -= Input::get_mouse_delta_x() * sensitivity;
		m_TargetEuler.x += Input::get_mouse_delta_y() * sensitivity;
		
		Matrix4 rotation = m_Transformation.get_rotation();
		Float3 forward = rotation * Float4(0.0f, 0.0f, -1.0f, 1.0f);
		Float3 up = rotation * Float4(0.0f, 1.0f, 0.0f, 1.0f);
		Float3 right = glm::cross(forward, up);

		if (Input::is_key_down(Key::W))
			m_TargetPosition += forward * moveSpeed;
		if (Input::is_key_down(Key::S))
			m_TargetPosition -= forward * moveSpeed;
		if (Input::is_key_down(Key::A))
			m_TargetPosition -= right * moveSpeed;
		if (Input::is_key_down(Key::D))
			m_TargetPosition += right * moveSpeed;
		if (Input::is_key_down(Key::Space))
			m_TargetPosition.y += moveSpeed;
		if (Input::is_key_down(Key::LShift))
			m_TargetPosition.y -= moveSpeed;
	}

	void CameraController::on_mouse_move(MouseMoveEvent& e)
	{
	}

	void CameraController::on_mouse_scroll(MouseScrollEvent& e)
	{
		if (m_Camera->get_projection_type() != ProjectionType::Perspective)
			return;

		m_MoveSpeed = std::min(std::max(m_MoveSpeed += (e.delta * 0.1f), 1.0f), 10.0f);
	}

	Matrix4 CameraController::get_view() const
	{
		return glm::inverse(m_Transformation.get_transform());
	}

	void CameraController::on_event(Event& e)
	{
		EventMessenger messenger(e);

		messenger.send<MouseMoveEvent>(DELEGATE(CameraController::on_mouse_move));
		messenger.send<MouseScrollEvent>(DELEGATE(CameraController::on_mouse_scroll));
	}

}