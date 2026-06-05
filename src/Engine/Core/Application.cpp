#include "Application.h"

#include "../Platform/OpenGL/Glad/glad.h"

#include <GLFW/glfw3.h>

Application::Application()
	: window_(1280, 720, "Mini Engine")
{
}

Application::~Application() = default;

int Application::Run() {
	while (is_running_ && !window_.ShouldClose()) {
		window_.PollEvents();

		Update();
		Render();

		window_.SwapBuffers();
	}

	return 0;
}

void Application::Update() {
	if (glfwGetKey(window_.GetNativeHandle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		is_running_ = false;
	}
}

void Application::Render() {
	glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}