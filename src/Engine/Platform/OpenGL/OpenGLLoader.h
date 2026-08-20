#pragma once


using OpenGLGetProcAddress =
	void* (*)(const char*);


bool InitializeOpenGLLoader(
	OpenGLGetProcAddress get_proc_address
);