#pragma once

#include "Engine/Math/matrix_types.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/Shader.h"


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

    void DrawLines(
        const Mesh& mesh,
        const Shader& shader,
        const Matrix4& mvp,
        float line_width = 1.0f
    );
};