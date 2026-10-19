#pragma once

#include "Engine/Math/matrix_types.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/RenderStats.h"
#include "Engine/Renderer/Shader.h"

#include <cstddef>

/**
 * @brief Низкоуровневый OpenGL Renderer Mini Engine.
 *
 * Renderer инкапсулирует основные операции,
 * непосредственно связанные с выполнением OpenGL rendering.
 *
 * Класс отвечает за:
 *
 * - базовую настройку состояния OpenGL;
 * - очистку Color Buffer и Depth Buffer;
 * - настройку viewport;
 * - обычный indexed rendering Mesh;
 * - instanced rendering;
 * - рендеринг линий;
 * - depth-only rendering для Shadow Map;
 * - переключение framebuffer;
 * - сбор статистики draw calls;
 * - сбор времени отдельных render passes.
 *
 * Renderer не владеет Scene
 * и не принимает решений о том,
 * какие SceneObject необходимо отрисовывать.
 *
 * Выбор объектов, подготовка transform matrices,
 * настройка lighting uniforms и организация render passes
 * выполняются внешними системами.
 */
class Renderer {
public:
    /**
     * @brief Инициализирует базовое состояние OpenGL.
     *
     * В текущей реализации включаются:
     *
     * - GL_DEPTH_TEST;
     * - GL_PROGRAM_POINT_SIZE.
     *
     * Метод должен вызываться
     * при активном OpenGL-контексте.
     */
    void Initialize();

    /**
     * @brief Сбрасывает статистику перед началом нового кадра.
     *
     * Метод очищает текущий RenderStats
     * и устанавливает RenderPass::None.
     */
    void BeginFrameStats();

    /**
     * @brief Устанавливает текущий этап рендеринга.
     *
     * Все последующие зарегистрированные draw calls
     * относятся к указанному RenderPass
     * до следующего вызова SetRenderPass().
     *
     * @param pass Текущий этап рендеринга.
     */
    void SetRenderPass(RenderPass pass);

    /**
     * @brief Добавляет время выполнения render pass.
     *
     * Время именно накапливается,
     * а не заменяет предыдущее значение.
     *
     * Это позволяет учитывать один логический этап,
     * состоящий из нескольких отдельных участков рендеринга.
     *
     * @param pass Этап рендеринга.
     * @param milliseconds Время выполнения в миллисекундах.
     */
    void AddRenderPassTime(RenderPass pass, double milliseconds);

    /**
     * @brief Возвращает статистику Renderer текущего кадра.
     *
     * Статистика содержит количество draw calls
     * и временные показатели отдельных render passes.
     *
     * @return Константная ссылка на RenderStats.
     */
    const RenderStats& GetRenderStats() const;

    /**
     * @brief Начинает обычный render frame.
     *
     * Метод:
     *
     * - устанавливает background color;
     * - очищает Color Buffer;
     * - очищает Depth Buffer.
     *
     * @param background_color Цвет фона кадра.
     */
    void BeginFrame(const Vec3& background_color);

    /**
     * @brief Устанавливает размер OpenGL viewport.
     *
     * Левая нижняя точка viewport
     * устанавливается в координаты (0, 0).
     *
     * @param width Ширина viewport.
     * @param height Высота viewport.
     */
    void SetViewport(int width, int height);

    /**
     * @brief Рисует Mesh треугольниками без instancing.
     *
     * Метод:
     *
     * - активирует shader;
     * - устанавливает uUseInstancing = 0;
     * - передаёт uMVP;
     * - привязывает Mesh;
     * - выполняет glDrawElements() с GL_TRIANGLES;
     * - регистрирует draw call.
     *
     * @param mesh Геометрия объекта.
     * @param shader Shader program.
     * @param mvp Model-View-Projection Matrix.
     */
    void Draw(const Mesh& mesh, const Shader& shader, const Matrix4& mvp);

    /**
     * @brief Рисует несколько экземпляров одного Mesh
     * одним instanced draw call.
     *
     * Перед вызовом Mesh должен содержать
     * instance model matrices,
     * загруженные через Mesh::SetInstanceMatrices().
     *
     * Shader получает:
     *
     * - uUseInstancing = 1;
     * - общую View-Projection Matrix.
     *
     * После выполнения glDrawElementsInstanced()
     * instance attributes Mesh отключаются,
     * а uUseInstancing возвращается в 0.
     *
     * Один вызов glDrawElementsInstanced()
     * учитывается статистикой как один draw call.
     *
     * @param mesh Общая геометрия instances.
     * @param shader Shader program с поддержкой instancing.
     * @param view_projection Общая View-Projection Matrix.
     * @param instance_count Количество экземпляров Mesh.
     */
    void DrawInstanced(const Mesh& mesh, const Shader& shader, const Matrix4& view_projection, std::size_t instance_count);

    /**
     * @brief Рисует Mesh как набор линий.
     *
     * Метод использует primitive type GL_LINES.
     *
     * Перед рендерингом:
     *
     * - устанавливается uUseInstancing = 0;
     * - передаётся uMVP;
     * - задаётся требуемая толщина линии.
     *
     * @param mesh Геометрия линий.
     * @param shader Shader program.
     * @param mvp Model-View-Projection Matrix.
     * @param line_width Толщина линии.
     */
    void DrawLines(const Mesh& mesh, const Shader& shader, const Matrix4& mvp, float line_width = 1.0f);

    /**
     * @brief Выполняет depth-only draw call.
     *
     * Метод используется при построении Shadow Map.
     *
     * В отличие от обычного Draw(),
     * Renderer здесь не устанавливает transform uniforms.
     *
     * Необходимые значения shadow shader,
     * включая используемые системой:
     *
     * - uModel;
     * - uLightVP,
     *
     * должны быть настроены внешним кодом
     * перед вызовом DrawDepth().
     *
     * Метод только:
     *
     * - активирует shader;
     * - привязывает Mesh;
     * - выполняет GL_TRIANGLES draw call;
     * - регистрирует draw call.
     *
     * @param mesh Геометрия, записываемая в shadow map.
     * @param shader Shadow shader.
     */
    void DrawDepth(const Mesh& mesh, const Shader& shader);

    /**
     * @brief Делает указанный framebuffer текущим.
     *
     * Переданный ID напрямую передаётся
     * в glBindFramebuffer(GL_FRAMEBUFFER, ...).
     *
     * Метод может использоваться,
     * например, для переключения между shadow framebuffer
     * и framebuffer основного viewport.
     *
     * @param framebuffer_id OpenGL ID framebuffer.
     */
    void BindFramebuffer(unsigned int framebuffer_id);

private:
    /**
     * @brief Регистрирует один выполненный OpenGL draw call.
     *
     * Метод всегда увеличивает общий счётчик draw_calls.
     *
     * Дополнительно, в зависимости от current_render_pass_,
     * увеличивается счётчик:
     *
     * - shadow_draw_calls;
     * - main_draw_calls;
     * - editor_draw_calls.
     */
    void RegisterDrawCall();

    /**
     * @brief Статистика рендеринга текущего кадра.
     */
    RenderStats render_stats_;

    /**
     * @brief Render pass, к которому относятся
     * последующие зарегистрированные draw calls.
     */
    RenderPass current_render_pass_ = RenderPass::None;
};