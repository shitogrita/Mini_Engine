#include "Renderer.h"

#include "../Platform/OpenGL/Glad/glad.h"
#include <algorithm>



void Renderer::BeginFrame(const Vec3& background_color) {
	glClearColor(
		background_color.x,
		background_color.y,
		background_color.z,
		1.0f
	);

	glClear(
		GL_COLOR_BUFFER_BIT |
		GL_DEPTH_BUFFER_BIT
	);
}

void Renderer::Draw(
	const Mesh& mesh,
	const Shader& shader,
	const Matrix4& mvp
) {
	shader.Use();
	shader.SetMat4("uMVP", mvp);
	mesh.Draw();

}