#pragma once

#include "Engine/Math/matrix_types.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/Shader.h"

/**
 * @brief Низкоуровневый OpenGL Renderer.
 *
 * Renderer отвечает за:
 * - базовую настройку OpenGL;
 * - очистку framebuffer;
 * - настройку viewport;
 * - обычный рендеринг Mesh;
 * - рендеринг линий;
 * - depth-only rendering для Shadow Map.
 *
 * Renderer не владеет Scene и не принимает решений
 * о том, какие SceneObject необходимо рисовать.
 */
class Renderer {
public:
    /**
     * @brief Инициализирует базовое состояние OpenGL.
     */
    void Initialize();

    /**
     * @brief Начинает обычный кадр.
     *
     * Очищает Color Buffer и Depth Buffer.
     *
     * @param background_color Цвет фона.
     */
    void BeginFrame(const Vec3& background_color);

    /**
     * @brief Устанавливает OpenGL viewport.
     *
     * @param width Ширина viewport.
     * @param height Высота viewport.
     */
    void SetViewport(int width, int height);

    /**
     * @brief Рисует Mesh треугольниками.
     *
     * Метод используется обычным render pass.
     *
     * @param mesh Геометрия объекта.
     * @param shader Shader, используемый для отрисовки.
     * @param mvp Model-View-Projection Matrix.
     */
    void Draw(const Mesh& mesh, const Shader& shader, const Matrix4& mvp);

    /**
     * @brief Рисует Mesh линиями.
     *
     * Используется для Grid, осей и Gizmo.
     *
     * @param mesh Геометрия линий.
     * @param shader Shader.
     * @param mvp Model-View-Projection Matrix.
     * @param line_width Толщина линии.
     */
    void DrawLines(const Mesh& mesh, const Shader& shader, const Matrix4& mvp, float line_width = 1.0f);

    /**
     * @brief Выполняет depth-only draw call.
     *
     * Метод используется при построении Point Shadow Map.
     *
     * В отличие от Draw(), здесь Renderer не устанавливает uMVP,
     * потому что shadow shader получает отдельно:
     *
     * - uModel;
     * - uLightVP.
     *
     * Эти uniforms будут установлены SceneViewport
     * перед вызовом DrawDepth().
     *
     * @param mesh Геометрия, записываемая в Depth Cubemap.
     * @param shader Shadow shader.
     */
    void DrawDepth(const Mesh& mesh, const Shader& shader);

    /**
	 * @brief Делает указанный framebuffer текущим.
	 *
	 * Используется после Shadow Pass для возврата
	 * к framebuffer, принадлежащему QOpenGLWidget.
	 *
	 * @param framebuffer_id OpenGL ID framebuffer.
	 */
    void BindFramebuffer(unsigned int framebuffer_id);
};