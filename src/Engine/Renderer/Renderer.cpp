#include "Renderer.h"

#include "../Platform/OpenGL/Glad/glad.h"


void Renderer::Initialize()
{
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_PROGRAM_POINT_SIZE);
}


void Renderer::BeginFrame(
	const Vec3& background_color
)
{
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


void Renderer::SetViewport(
	int width,
	int height
)
{
	glViewport(
		0,
		0,
		width,
		height
	);
}


void Renderer::Draw(
	const Mesh& mesh,
	const Shader& shader,
	const Matrix4& mvp
)
{
	shader.Use();

	shader.SetMat4(
		"uMVP",
		mvp
	);

	mesh.Bind();

	glDrawElements(
		GL_TRIANGLES,
		static_cast<GLsizei>(
			mesh.GetIndexCount()
		),
		GL_UNSIGNED_INT,
		nullptr
	);

	glBindVertexArray(0);
}


void Renderer::DrawLines(
	const Mesh& mesh,
	const Shader& shader,
	const Matrix4& mvp,
	float line_width
)
{
	shader.Use();

	shader.SetMat4(
		"uMVP",
		mvp
	);

	mesh.Bind();

	glLineWidth(
		line_width
	);

	glDrawElements(
		GL_LINES,
		static_cast<GLsizei>(
			mesh.GetIndexCount()
		),
		GL_UNSIGNED_INT,
		nullptr
	);

	glBindVertexArray(0);
}