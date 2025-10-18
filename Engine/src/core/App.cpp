#include "pch.h"

#include "windowing/Window.h"
#include "App.h"

#include "rendering/Graphics.h"

namespace Engine {

	App* App::m_Instance = nullptr;

	App::App(const std::string& window_name, uint32_t window_width, uint32_t window_height)
	{
		ASSERT(!m_Instance);

		m_Instance = this;

		m_Window = make_owning<Window>(window_name, window_width, window_height);
	}

	App::~App()
	{
	}	

	void App::run()
	{
		Graphics::init();

		Hooks.start(*this);
		
		auto startTime = std::chrono::high_resolution_clock::now();
		float lastElapsed = 0.0f;

		while (m_Window->is_open())
		{
			Input::clear_state();
			Input::update_mouse_delta();
			m_Window->handle_events();

			if (!m_Window->is_minimized())
			{
				Hooks.update(*this);
				m_FrameNumber++;
			}

			// Time
			auto current = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> elapsedDuration = current - startTime;
			m_DeltaTime = elapsedDuration.count() - lastElapsed;
			m_ElapsedTime += m_DeltaTime;
			lastElapsed = elapsedDuration.count();

			m_Window->swap_buffers();
		}
	}

	void App::close()
	{
		Hooks.stop(*this);
	}

	bool App::is_running() const
	{
		return m_Window->is_open();
	}

	Window* App::get_window() const
	{
		return m_Window.get();
	}

	void App::handle_event(Event& event)
	{
		EventMessenger messenger(event);
		messenger.send<WindowResizeEvent>(DELEGATE(App::on_window_resize));

		messenger.send<KeyPressEvent>(DELEGATE(App::on_key_press));
		messenger.send<KeyReleaseEvent>(DELEGATE(App::on_key_release));

		messenger.send<MouseButtonPressEvent>(DELEGATE(App::on_mouse_button_press));
		messenger.send<MouseButtonReleaseEvent>(DELEGATE(App::on_mouse_button_release));
		messenger.send<MouseMoveEvent>(DELEGATE(App::on_mouse_move));
		messenger.send<MouseScrollEvent>(DELEGATE(App::on_mouse_scroll));

		if (Hooks.event)
			Hooks.event(event);
	}

	void App::on_window_resize(WindowResizeEvent& e)
	{
		Graphics::resize_viewport(e.width, e.height);
	}

	void App::on_key_press(KeyPressEvent& e)
	{
		Input::s_KeysDown[(uint32_t)e.key] = 1;
		if (!e.repeat)
			Input::s_KeysPressed[(uint32_t)e.key] = 1;
	}
	void App::on_key_release(KeyReleaseEvent& e)
	{
		Input::s_KeysDown[(uint32_t)e.key] = 0;
		Input::s_KeysReleased[(uint32_t)e.key] = 1;
	}
	void App::on_mouse_button_press(MouseButtonPressEvent& e)
	{
		bool first = Input::s_KeysDown[(uint32_t)e.button] == 0;
		Input::s_KeysDown[(uint32_t)e.button] = 1;
		if (first)
			Input::s_KeysPressed[(uint32_t)e.button] = 1;
	}
	void App::on_mouse_button_release(MouseButtonReleaseEvent& e)
	{
		Input::s_KeysDown[(uint32_t)e.button] = 0;
		Input::s_KeysReleased[(uint32_t)e.button] = 1;
	}
	void App::on_mouse_move(MouseMoveEvent& e)
	{
		Input::s_MouseX = (float)e.x;
		Input::s_MouseY = (float)e.y;
	}
	void App::on_mouse_scroll(MouseScrollEvent& e)
	{
	}

}