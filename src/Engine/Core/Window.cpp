#include "Window.h"
#include "../Platform/OpenGL/Glad/glad.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

Window::Window(int width, int height, const char *title)
	: width_(width),
	  height_(height)
	{
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialize GLFW");
	}

	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	handle_ = glfwCreateWindow(width_, height_, title, nullptr, nullptr); //указатель на окно GLFM
	if (!handle_) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(handle_); // контекст - это состояние OpenGL

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { // Загрузка адресов функций OpenGL
		glfwDestroyWindow(handle_);
		glfwTerminate();
		throw std::runtime_error("Failed to initialize GLAD");
	}

	glViewport(0, 0, width_, height_);

	glfwSetWindowUserPointer(handle_, this);

	glfwSetFramebufferSizeCallback(
		handle_,
		[](GLFWwindow* window, int new_width, int new_height) {
			auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
			self->width_ = new_width;
			self->height_ = new_height;

			glViewport(0, 0, new_width, new_height);
		}
	);
	glEnable(GL_DEPTH_TEST);

	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
}

Window::~Window() {
	if (handle_) {
		glfwDestroyWindow(handle_);
		handle_ = nullptr;
	}
	glfwTerminate();
}

/*
void Window::processInput(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void Window::framebuffer_size_callback(GLFWwindow *window, int width, int height) { // так как это от с пришло и работает с обычными C-callback
	// подтяжка окна под изменения
	glViewport(0, 0, width, height);
}
*/

bool Window::ShouldClose() const {
	return glfwWindowShouldClose(handle_);
}

void Window::PollEvents() {
	glfwPollEvents();
}

void Window::SwapBuffers() {
	glfwSwapBuffers(handle_);
}

int Window::GetWidth() const {
	return width_;
}

int Window::GetHeight() const {
	return height_;
}

float Window::GetAspectRatio() const{
	if (height_ == 0) {
		return 1.0f;
	}

	return static_cast<float>(width_) / static_cast<float>(height_);
}

GLFWwindow* Window::GetNativeHandle() {
	return handle_;
}