#include "Engine/Renderer/Renderer.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"

/**
 * @brief Инициализирует базовое состояние OpenGL Renderer.
 *
 * Метод включает состояния OpenGL,
 * необходимые текущей системе рендеринга:
 *
 * - GL_DEPTH_TEST — проверку глубины;
 * - GL_PROGRAM_POINT_SIZE — возможность задавать размер
 *   точек непосредственно из shader program.
 *
 * Метод должен вызываться при активном OpenGL-контексте.
 */
void Renderer::Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
}

/**
 * @brief Сбрасывает статистику Renderer перед новым кадром.
 *
 * Метод:
 *
 * - очищает накопленные RenderStats;
 * - устанавливает текущий render pass в RenderPass::None.
 *
 * После вызова статистика нового кадра
 * начинает собираться с нулевых значений.
 */
void Renderer::BeginFrameStats() {
    render_stats_.Reset();
    current_render_pass_ = RenderPass::None;
}

/**
 * @brief Устанавливает текущий этап рендеринга.
 *
 * Значение current_render_pass_ используется
 * при регистрации draw calls,
 * чтобы отдельно учитывать количество вызовов
 * для Shadow, Main и Editor pass.
 *
 * @param pass Текущий этап рендеринга.
 */
void Renderer::SetRenderPass(RenderPass pass) {
    current_render_pass_ = pass;
}

/**
 * @brief Добавляет время выполнения указанного render pass.
 *
 * В зависимости от значения pass
 * время накапливается в соответствующем поле RenderStats:
 *
 * - Shadow -> shadow_ms;
 * - Main -> main_ms;
 * - Editor -> editor_ms.
 *
 * Для RenderPass::None значение не записывается.
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
 *
 * @return Константная ссылка на текущий RenderStats.
 */
const RenderStats& Renderer::GetRenderStats() const {
    return render_stats_;
}

/**
 * @brief Регистрирует один выполненный draw call.
 *
 * Общий счётчик draw_calls увеличивается
 * независимо от текущего render pass.
 *
 * Дополнительно увеличивается счётчик,
 * соответствующий current_render_pass_:
 *
 * - shadow_draw_calls;
 * - main_draw_calls;
 * - editor_draw_calls.
 *
 * Если текущий pass равен RenderPass::None,
 * увеличивается только общий draw_calls.
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
 * Метод устанавливает цвет очистки Color Buffer,
 * после чего очищает:
 *
 * - Color Buffer;
 * - Depth Buffer.
 *
 * Alpha-компонента background color
 * всегда устанавливается равной 1.0f.
 *
 * @param background_color Цвет фона кадра.
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
 * @brief Устанавливает размер OpenGL viewport.
 *
 * Начальная точка viewport всегда находится
 * в координатах (0, 0).
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
 * @brief Рисует Mesh треугольниками без instancing.
 *
 * Метод:
 *
 * - активирует shader;
 * - отключает режим instancing через uUseInstancing;
 * - передаёт Model-View-Projection Matrix в uMVP;
 * - привязывает VAO Mesh;
 * - выполняет indexed draw call через glDrawElements();
 * - регистрирует draw call в статистике;
 * - снимает binding VAO.
 *
 * Геометрия интерпретируется как GL_TRIANGLES,
 * а индексы — как GL_UNSIGNED_INT.
 *
 * @param mesh Геометрия объекта.
 * @param shader Shader program для рендеринга.
 * @param mvp Model-View-Projection Matrix.
 */
void Renderer::Draw(const Mesh& mesh, const Shader& shader, const Matrix4& mvp) {
    shader.Use();

    /**
     * Обычный Draw() не использует
     * instance model matrices.
     */
    shader.SetInt(
        "uUseInstancing",
        0
    );

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
 * @brief Рисует Mesh как набор линий.
 *
 * Метод работает аналогично обычному Draw(),
 * но использует primitive type GL_LINES.
 *
 * Перед draw call также устанавливается
 * требуемая толщина линии через glLineWidth().
 *
 * Instancing для данного вызова отключается
 * через uniform uUseInstancing = 0.
 *
 * @param mesh Геометрия линий.
 * @param shader Shader program.
 * @param mvp Model-View-Projection Matrix.
 * @param line_width Толщина линии.
 */
void Renderer::DrawLines(const Mesh& mesh, const Shader& shader, const Matrix4& mvp, float line_width) {
    shader.Use();

    shader.SetInt(
        "uUseInstancing",
        0
    );

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
 * Renderer не формирует transform uniforms
 * для shadow shader внутри этого метода.
 *
 * До вызова DrawDepth() shader уже должен содержать
 * необходимые значения, включая используемые системой:
 *
 * - uModel;
 * - uLightVP.
 *
 * Метод:
 *
 * - активирует shadow shader;
 * - привязывает Mesh;
 * - выполняет indexed GL_TRIANGLES draw call;
 * - регистрирует draw call;
 * - снимает binding VAO.
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
 * @brief Привязывает framebuffer для последующего рендеринга.
 *
 * Переданный OpenGL framebuffer ID
 * устанавливается как текущий GL_FRAMEBUFFER.
 *
 * Метод не предполагает,
 * что framebuffer_id = 0 обязательно является
 * framebuffer конкретного SceneViewport.
 *
 * @param framebuffer_id OpenGL ID framebuffer.
 */
void Renderer::BindFramebuffer(unsigned int framebuffer_id) {
    glBindFramebuffer(
        GL_FRAMEBUFFER,
        framebuffer_id
    );
}

/**
 * @brief Выполняет instanced rendering одного Mesh.
 *
 * Один indexed draw call используется
 * для отрисовки нескольких экземпляров одной геометрии.
 *
 * Метод предполагает, что instance matrices
 * уже были загружены в Mesh
 * через Mesh::SetInstanceMatrices().
 *
 * Последовательность работы:
 *
 * - проверяется количество instances;
 * - активируется shader;
 * - включается режим instancing через uUseInstancing;
 * - передаётся View-Projection Matrix;
 * - привязывается Mesh;
 * - выполняется glDrawElementsInstanced();
 * - регистрируется один draw call;
 * - отключаются instance attributes 4-7;
 * - uUseInstancing возвращается в 0.
 *
 * Несмотря на количество instances,
 * вызов glDrawElementsInstanced() учитывается
 * как один draw call.
 *
 * Если instance_count равен 0,
 * OpenGL-вызовы и регистрация draw call не выполняются.
 *
 * @param mesh Геометрия, используемая всеми instances.
 * @param shader Shader program с поддержкой instancing.
 * @param view_projection View-Projection Matrix.
 * @param instance_count Количество экземпляров Mesh.
 */
void Renderer::DrawInstanced(const Mesh& mesh, const Shader& shader, const Matrix4& view_projection, std::size_t instance_count) {
    if (instance_count == 0) {
        return;
    }

    shader.Use();

    /**
     * Сообщаем shader,
     * что Model Matrix должна быть получена
     * из instance vertex attributes.
     */
    shader.SetInt(
        "uUseInstancing",
        1
    );

    /**
     * Общая View-Projection Matrix
     * используется всеми instances.
     */
    shader.SetMat4(
        "uViewProjection",
        view_projection
    );

    mesh.Bind();

    glDrawElementsInstanced(
        GL_TRIANGLES,
        static_cast<GLsizei>(
            mesh.GetIndexCount()
        ),
        GL_UNSIGNED_INT,
        nullptr,
        static_cast<GLsizei>(
            instance_count
        )
    );

    /**
     * Один glDrawElementsInstanced()
     * является одним draw call.
     */
    RegisterDrawCall();

    /**
     * После instanced draw отключаем attributes 4-7.
     *
     * Это возвращает VAO Mesh
     * к состоянию без активных instance attributes.
     */
    mesh.DisableInstanceAttributes();

    /**
     * Возвращаем shader
     * в обычный режим рендеринга.
     */
    shader.SetInt(
        "uUseInstancing",
        0
    );
}