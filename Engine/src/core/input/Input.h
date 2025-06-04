#pragma once

#include "InputCodes.h"

namespace Engine { 

	class Input
	{
	public:
		static bool is_key_down(Key key);
		static bool was_key_pressed(Key key);
		static bool was_key_released(Key key);

		static bool is_button_down(MouseButton button);
		static bool was_button_pressed(MouseButton button);
		static bool was_button_released(MouseButton button);

		static float get_mouse_x() { return s_MouseX; }
		static float get_mouse_y() { return s_MouseY; }

		static float get_mouse_delta_x() { return s_MouseDeltaX; }
		static float get_mouse_delta_y() { return s_MouseDeltaY; }

		static CursorMode get_cursor_mode();
		static void set_cursor_mode(CursorMode mode);
	private:
		static void update_mouse_delta();
		static void clear_state(bool fully = false); // fully = whether or not to clear s_KeysDown and s_ButtonsDown
	private:
		static float s_MouseX, s_MouseY;
		static float s_MouseDeltaX, s_MouseDeltaY;

		static std::bitset<(uint32_t)Key::COUNT> s_KeysDown, s_KeysPressed, s_KeysReleased;
		static std::bitset<(uint32_t)MouseButton::COUNT> s_ButtonsDown, s_ButtonsPressed, s_ButtonsReleased;

		friend class App;
		friend class Window;
	};

}