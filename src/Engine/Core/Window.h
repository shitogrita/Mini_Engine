#pragma once

struct GLFWwindow;


class Window {
  public:
	Window(int width, int height, const char* title);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	bool ShouldClose() const;
	void PollEvents();
	void SwapBuffers();

	int GetWidth() const;
	int GetHeight() const;
	float GetAspectRatio() const;

	GLFWwindow* GetNativeHandle();

	void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	void processInput(GLFWwindow *window);

  private:
	GLFWwindow* handle_ = nullptr;
	int width_ = 0;
	int height_ = 0;
};
