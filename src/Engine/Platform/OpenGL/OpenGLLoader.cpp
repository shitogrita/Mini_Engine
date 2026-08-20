#include "Engine/Platform/OpenGL/OpenGLLoader.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"


bool InitializeOpenGLLoader(
	OpenGLGetProcAddress get_proc_address
)
{
	return gladLoadGLLoader(
		reinterpret_cast<GLADloadproc>(
			get_proc_address
		)
	) != 0;
}