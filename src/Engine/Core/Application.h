#pragma once

#include "Window.h"

class Application {
  public:
	Application();
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	int Run();

  private:
	void Update();
	void Render();

  private:
	Window window_;
	bool is_running_ = true;
};