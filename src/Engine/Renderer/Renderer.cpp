#include "Engine/Renderer/Renderer.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"

/**
 * @brief Инициализирует базовое состояние OpenGL Renderer.
 */
void Renderer::Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
}

/**
 * @brief Сбрасывает статистику перед началом нового кадра.
 */
void Renderer::BeginFrameStats() {
    render_stats_.Reset();
    current_render_pass_ = RenderPass::None;
}

/**
 * @brief Устанавливает текущий этап рендеринга.
 *
 * @param pass Текущий этап рендеринга.
 */
void Renderer::SetRenderPass(RenderPass pass) {
    current_render_pass_ = pass;
}

/**
 * @brief Добавляет время выполнения render pass.
 *
 * @param pass Этап рендеринга.
 * @param milliseconds Время выполнения в миллисекундах.
 */
void Renderer::AddRenderPassTime(RenderPass pass, double milliseconds) {
    switch (pass) {
        case RenderPass::Shadow:
            render_stats_.shadow_ms += milliseconds;
            break;

        case RenderPass::Main:
            render_stats_.main_ms += milliseconds;
            break;

        case RenderPass::Editor:
            render_stats_.editor_ms += milliseconds;
            break;

        case RenderPass::None:
            break;
    }
}

/**
 * @brief Возвращает статистику Renderer текущего кадра.
 */
const RenderStats& Renderer::GetRenderStats() const {
    return render_stats_;
}

/**
 * @brief Регистрирует один выполненный draw call.
 */
void Renderer::RegisterDrawCall() {
    ++render_stats_.draw_calls;

    switch (current_render_pass_) {
        case RenderPass::Shadow:
            ++render_stats_.shadow_draw_calls;
            break;

        case RenderPass::Main:
            ++render_stats_.main_draw_calls;
            break;

        case RenderPass::Editor:
            ++render_stats_.editor_draw_calls;
            break;

        case RenderPass::None:
            break;
    }
}

/**
 * @brief Начинает обычный render frame.
 *
 * @param background_color Цвет очистки Color Buffer.
 */
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

/**
 * @brief Изменяет OpenGL viewport.
 *
 * @param width Ширина viewport.
 * @param height Высота viewport.
 */
void Renderer::SetViewport(int width, int height) {
    glViewport(
        0,
        0,
        width,
        height
    );
}

/**
 * @brief Рисует Mesh треугольниками обычным shader.
 *
 * @param mesh Геометрия объекта.
 * @param shader Shader program.
 * @param mvp Model-View-Projection Matrix.
 */
void Renderer::Draw(const Mesh& mesh, const Shader& shader, const Matrix4& mvp) {
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

    RegisterDrawCall();

    glBindVertexArray(0);
}

/**
 * @brief Рисует Mesh линиями.
 *
 * @param mesh Геометрия линий.
 * @param shader Shader program.
 * @param mvp Model-View-Projection Matrix.
 * @param line_width Толщина линии.
 */
void Renderer::DrawLines(const Mesh& mesh, const Shader& shader, const Matrix4& mvp, float line_width) {
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

    RegisterDrawCall();

    glBindVertexArray(0);
}

/**
 * @brief Выполняет depth-only draw call для Shadow Map.
 *
 * Shadow shader уже должен содержать корректные:
 *
 * uModel
 * uLightVP
 *
 * Renderer здесь только активирует shader,
 * привязывает Mesh и выполняет glDrawElements().
 *
 * @param mesh Геометрия объекта.
 * @param shader Shadow shader.
 */
void Renderer::DrawDepth(const Mesh& mesh, const Shader& shader) {
    shader.Use();

    mesh.Bind();

    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(
            mesh.GetIndexCount()
        ),
        GL_UNSIGNED_INT,
        nullptr
    );

    RegisterDrawCall();

    glBindVertexArray(0);
}

/**
 * @brief Привязывает framebuffer для последующего rendering.
 *
 * @param framebuffer_id OpenGL ID framebuffer.
 */
void Renderer::BindFramebuffer(unsigned int framebuffer_id) {
    glBindFramebuffer(
        GL_FRAMEBUFFER,
        framebuffer_id
    );
}