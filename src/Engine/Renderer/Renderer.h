#pragma once


#include "../Math/matrix_types.h"
#include "Mesh.h"
#include "Shader.h"

class Renderer {
  public:
	void BeginFrame(const RenderParams& params);
	void Draw(
	const Mesh& mesh,
	const Shader& shader,
	const Matrix4& mvp);

  private:
};
