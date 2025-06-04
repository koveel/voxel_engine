#pragma once

namespace Engine {

	class Window
	{
	public:
		Window(const std::string& title, uint32_t width, uint32_t height);
		~Window();
	
		void set_title(const std::string& title);
		void set_shown(bool show);
		bool is_open() const { return m_Open; }
		bool is_minimized() const;
		void close();

		void set_icon(const std::string& icon_path);
		void set_fullscreen(bool fs);

		uint32_t get_width() const { return m_Width; }
		uint32_t get_height() const { return m_Height; }

		struct GLFWwindow* get_handle() const { return m_Handle; }

		void resize(uint32_t width, uint32_t height);
	private:
		void handle_events();
		void swap_buffers();
	private:
		GLFWwindow* m_Handle;
		uint32_t m_Width = 0, m_Height = 0;
		bool m_Open = false;

		friend class App;
	};
	
}