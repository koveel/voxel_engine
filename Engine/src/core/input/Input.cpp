#include "pch.h"

#define GLFW_INCLUDE_NONE
#include <glfw/glfw3.h>

#include "windowing/Window.h"
#include "App.h"

namespace Engine {

	std::bitset<(uint32_t)Key::COUNT> Input::s_KeysDown, Input::s_KeysPressed, Input::s_KeysReleased;
	std::bitset<(uint32_t)MouseButton::COUNT> Input::s_ButtonsDown, Input::s_ButtonsPressed, Input::s_ButtonsReleased;
	float Input::s_MouseX = 0.0f, Input::s_MouseY = 0.0f;
	float Input::s_MouseDeltaX = 0.0f, Input::s_MouseDeltaY = 0.0f;

	bool Input::is_key_down(Key key)
	{
		return s_KeysDown[(uint32_t)key];
	}

	bool Input::was_key_pressed(Key key)
	{
		return s_KeysPressed[(uint32_t)key];
	}

	bool Input::was_key_released(Key key)
	{
		return s_KeysReleased[(uint32_t)key];
	}

	bool Input::is_button_down(MouseButton button)
	{
		return s_ButtonsDown[(uint32_t)button];
	}

	bool Input::was_button_pressed(MouseButton button)
	{
		return s_ButtonsPressed[(uint32_t)button];
	}

	bool Input::was_button_released(MouseButton button)
	{
		return s_ButtonsReleased[(uint32_t)button];
	}

	CursorMode Input::get_cursor_mode()
	{
		Window* window = App::get().get_window();
		return (CursorMode)glfwGetInputMode(window->get_handle(), GLFW_CURSOR);
	}

	void Input::set_cursor_mode(CursorMode mode)
	{
		GLFWwindow* windowHandle = App::get().get_window()->get_handle();
		glfwSetInputMode(windowHandle, GLFW_CURSOR, (int)mode);
	}

	void Input::update_mouse_delta()
	{
		static float lastX = 0.0f, lastY = 0.0f;

		s_MouseDeltaX = s_MouseX - lastX;
		s_MouseDeltaY = -(s_MouseY - lastY);

		lastX = s_MouseX;
		lastY = s_MouseY;
	}

	void Input::clear_state(bool fully)
	{
		if (fully)
		{
			s_KeysDown.reset();
			s_ButtonsDown.reset();
		}

		s_KeysPressed.reset();
		s_KeysReleased.reset();

		s_ButtonsPressed.reset();
		s_ButtonsReleased.reset();
	}

}