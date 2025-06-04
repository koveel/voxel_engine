#pragma once

#include "Camera.h"

namespace Engine {

	class CameraController
	{
	public:
		void update(float ts);
		void on_event(Event&);
		
		void on_mouse_move(MouseMoveEvent&);
		void on_mouse_scroll(MouseScrollEvent&);

		void smooth(float ts);

		Matrix4 get_view() const;
		Transformation& get_transform() { return m_Transformation; }
	public:
		Transformation m_Transformation =
		{
			{ 0.0f, 0.0f, 5.0f }
		};
		Camera* m_Camera = nullptr;

		float m_MoveSpeed = 4.0f;
		Float3 m_TargetPosition = m_Transformation.Position, m_TargetEuler = m_Transformation.Rotation;
	};

}