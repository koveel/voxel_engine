#pragma once

namespace Engine {

	class App
	{
	public:
		App(const std::string& window_name, uint32_t window_width, uint32_t window_height);
		~App();

		void run();
		void close();

		bool is_running() const;

		void handle_event(Event& e);
		void on_window_resize(WindowResizeEvent& e);
		void on_key_press(KeyPressEvent& e);
		void on_key_release(KeyReleaseEvent&);
		void on_mouse_button_press(MouseButtonPressEvent&);
		void on_mouse_button_release(MouseButtonReleaseEvent&);
		void on_mouse_move(MouseMoveEvent&);
		void on_mouse_scroll(MouseScrollEvent&);

		class Window* get_window() const;

		static App& get() { return *m_Instance; }
	private:
		std::unique_ptr<Window> m_Window;

		float m_DeltaTime = 0.0f, m_ElapsedTime = 0.0f;

		static App* m_Instance;
	};

}