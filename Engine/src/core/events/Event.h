#pragma once

#include "input/InputCodes.h"

namespace Engine {

	class Window;

	enum class EventType
	{
		// Windowing
		WindowClose, WindowMove, WindowResize,
		WindowFocusChange,
		// Input
		KeyPress, KeyRelease, KeyType, MouseButtonPress, MouseButtonRelease,
		MouseMove, MouseScroll,
	};

#define EVENT_BODY(format) static EventType get_static_type() { return EventType::format; }\
							EventType get_type() const override { return EventType::format; }

	struct Event
	{
		virtual EventType get_type() const = 0;
	};

	struct KeyPressEvent : public Event
	{
		Window* window = nullptr;
		bool repeat = false;
		Key key;

		EVENT_BODY(KeyPress)

		KeyPressEvent(Key key, bool repeat, Window* window)
			: key(key), repeat(repeat), window(window)
		{
		}
	};

	struct KeyReleaseEvent : public Event
	{
		Window* window = nullptr;
		Key key;

		EVENT_BODY(KeyRelease)

		KeyReleaseEvent(Key key, Window* window)
			: key(key), window(window)
		{
		}
	};

	struct KeyTypeEvent : public Event
	{
		Window* window = nullptr;
		bool repeat = false;
		char character;
		Key key;

		EVENT_BODY(KeyType)

		KeyTypeEvent(Key key, char c, bool repeat, Window* window)
			: key(key), character(c), repeat(repeat), window(window)
		{
		}
	};

	struct MouseButtonPressEvent : public Event
	{
		Window* window = nullptr;
		Key button;

		EVENT_BODY(MouseButtonPress)

		MouseButtonPressEvent(Key button, Window* window)
			: button(button), window(window)
		{
		}
	};

	struct MouseButtonReleaseEvent : public Event
	{
		Window* window = nullptr;
		Key button;

		EVENT_BODY(MouseButtonRelease)

		MouseButtonReleaseEvent(Key button, Window* window)
			: button(button), window(window)
		{
		}
	};

	struct MouseMoveEvent : public Event
	{
		Window* window = nullptr;
		float x = 0.0f, y = 0.0f;

		EVENT_BODY(MouseMove)

		MouseMoveEvent(float x, float y, Window* window)
			: x(x), y(y), window(window)
		{
		}
	};

	struct MouseScrollEvent : public Event
	{
		Window* window = nullptr;
		float delta = 0.0f;
		float x = 0.0f, y = 0.0f;

		EVENT_BODY(MouseScroll)

		MouseScrollEvent(float delta, float x, float y, Window* window)
			: delta(delta), x(x), y(y), window(window)
		{
		}
	};

	struct WindowCloseEvent : public Event
	{
		Window* window = nullptr;

		EVENT_BODY(WindowClose)

		WindowCloseEvent(Window* window)
			: window(window)
		{
		}
	};

	struct WindowFocusChangeEvent : public Event
	{
		Window* window = nullptr;
		bool focused = false;

		EVENT_BODY(WindowFocusChange)

		WindowFocusChangeEvent(bool focused, Window* window)
			: focused(focused), window(window)
		{}
	};

	struct WindowMoveEvent : public Event
	{
		Window* window = nullptr;
		uint32_t x = 0, y = 0;

		EVENT_BODY(WindowMove)

		WindowMoveEvent(uint32_t x, uint32_t y, Window* window)
			: x(x), y(y), window(window)
		{
		}
	};

	struct WindowResizeEvent : public Event
	{
		Window* window = nullptr;
		uint32_t width = 0, height = 0;

		EVENT_BODY(WindowResize)

		WindowResizeEvent(uint32_t width, uint32_t height, Window* window)
			: width(width), height(height), window(window)
		{
		}
	};


#undef EVENT_BODY

}