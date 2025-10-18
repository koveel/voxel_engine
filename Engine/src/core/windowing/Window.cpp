#include "pch.h"

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <glfw/glfw3.h>

#include "Window.h"
#include "App.h"

namespace Engine {

	static void glfw_error_callback(int error, const char* description)
	{
		LOG("GLFW error: {}", description);
		ASSERT(false);
	}

	// Window callbacks
	static void callback_window_close(GLFWwindow* handle)
	{
		Window* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
		window->close();
	}

	static void callback_window_resize(GLFWwindow* handle, int width, int height)
	{
		Window* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
		window->resize(width, height);

		WindowResizeEvent e{ (uint32_t)width, (uint32_t)height, window };
		App::get().handle_event(e);
	}

	static void callback_key(GLFWwindow* handle, int key, int scancode, int action, int mods)
	{
		Window* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
		App& app = App::get();

		if (key == -1)
			return;

		// Press
		if (action == 1)
		{
			KeyPressEvent e{ (Key)key, false, window };
			app.handle_event(e);
		}
		// Release
		else if (action == 0)
		{
			KeyReleaseEvent e{ (Key)key, window };
			app.handle_event(e);
		}
		// Repeat
		else if (action == 2)
		{
			KeyPressEvent e{ (Key)key, true, window };
			app.handle_event(e);
		}
	}

	static void callback_mouse_button(GLFWwindow* handle, int button, int action, int mods)
	{
		Window* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
		App& app = App::get();

		// Press / repeat
		if (action == 1 || action == 2)
		{
			MouseButtonPressEvent e{ (Key)button, window };
			app.handle_event(e);
		}
		// Release
		else if (action == 0)
		{
			MouseButtonReleaseEvent e{ (Key)button, window };
			app.handle_event(e);
		}
	}

	static void callback_mouse_move(GLFWwindow* handle, double x, double y)
	{
		Window* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));

		MouseMoveEvent e = { (float)x, (float)y, window };
		App::get().handle_event(e);
	}

	static void callback_mouse_scroll(GLFWwindow* handle, double x, double y)
	{
		Window* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));

		double xpos, ypos;
		glfwGetCursorPos(handle, &xpos, &ypos);
		MouseScrollEvent e = { (float)y, (float)xpos, (float)ypos, window };
		App::get().handle_event(e);
	}

	Window::Window(const std::string& title, uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
		static bool window_created = false;
		ASSERT(!window_created);
		glfwSetErrorCallback(glfw_error_callback);
		ASSERT(glfwInit() && "failed to initialize glfw");

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		m_Handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		ASSERT(m_Handle && "failed to create window");
		glfwMakeContextCurrent(m_Handle);

		int gladResult = gladLoadGL();
		ASSERT(gladResult && "failed to initialize glad");

		glfwShowWindow(m_Handle);
		m_Open = true;

		glfwSwapInterval(1);
		glfwSetWindowUserPointer(m_Handle, this);

		//int monitorCount = 0;
		//GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

		// Set callbacks
		glfwSetWindowCloseCallback(m_Handle, callback_window_close);
		glfwSetWindowSizeCallback(m_Handle, callback_window_resize);
		glfwSetKeyCallback(m_Handle, callback_key);
		glfwSetMouseButtonCallback(m_Handle, callback_mouse_button);
		glfwSetCursorPosCallback(m_Handle, callback_mouse_move);
		glfwSetScrollCallback(m_Handle, callback_mouse_scroll);

		// Dispatch an initial window resize event (for cameras and whatnot)
		WindowResizeEvent e{ m_Width, m_Height, this };
		App::get().handle_event(e);
	}

	Window::~Window()
	{
		close();

		glfwDestroyWindow(m_Handle);
		glfwTerminate();
	}

	void Window::handle_events()
	{
		glfwPollEvents();
	}

	void Window::swap_buffers()
	{
		glfwSwapBuffers(m_Handle);
	}

	void Window::set_title(const std::string& title)
	{
		glfwSetWindowTitle(m_Handle, title.c_str());
	}

	void Window::set_icon(const std::string& path)
	{
	}

	void Window::set_fullscreen(bool fs)
	{
		if (!fs) {
			glfwSetWindowMonitor(m_Handle, nullptr, 0, 0, 0, 0, 0);
			return;
		}

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwSetWindowMonitor(m_Handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}

	void Window::set_shown(bool show)
	{
	}

	void Window::resize(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;

		glfwSetWindowSize(m_Handle, width, height);
	}

	bool Window::is_minimized() const
	{
		return m_Width == 0 && m_Height == 0;
	}

	void Window::close()
	{
		m_Open = false;
	}

}