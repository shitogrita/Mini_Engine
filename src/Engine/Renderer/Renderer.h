#pragma once

#include "../Math/matrix_types.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/Shader.h"


class Renderer {
public:
    void Initialize();

    void BeginFrame(
        const Vec3& background_color
    );

    void SetViewport(
        int width,
        int height
    );

    void Draw(
        const Mesh& mesh,
        const Shader& shader,
        const Matrix4& mvp
    );
};