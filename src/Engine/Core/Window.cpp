#include "Window.h"
#include "../Platform/OpenGL/Glad/glad.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

Window::Window(int width, int height, const char *title) {
	: width_(width),
	  height_(height)
	{
		if (!glfwInit()) {
			std::cout << "Failed to initialize GLFW" << std::endl;
			return -1;
		}


	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	GLFWwindow* window = glfwCreateWindow(800, 600, "Tets ?", nullptr, nullptr);
	handle_ = glfwCreateWindow(width_, height_, title, nullptr, nullptr);
	if (!handle_) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(handle_);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		glfwDestroyWindow(handle_);
		glfwTerminate();
		throw std::runtime_error("Failed to initialize GLAD");
	}


	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { // Загрузка адресов функций OpenGL
		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // вызов при каждом изменении окна
	//glViewport(0, 0, 800, 600); // от -1 до

	// render loop
	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		glfwSwapBuffers(window); // замена буфера цвета (пиксель за пикселем (двойной буфер))'
		glfwPollEvents();
	}

	 return 0;
}

void Window::processInput(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void Window::framebuffer_size_callback(GLFWwindow *window, int width, int height) {
	// подтяжка окна под изменения
	glViewport(0, 0, width, height);
}

int Window::GetWidth() const {
	return width_;
}

int Window::GetHeight() const {
	return height_;
}
