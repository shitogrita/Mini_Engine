#pragma once

#include <cstddef>

/**
 * @brief Определяет текущий этап рендеринга.
 *
 * RenderPass используется Renderer для разделения
 * статистики по отдельным этапам формирования кадра.
 *
 * Это позволяет отдельно учитывать:
 *
 * - Shadow Pass;
 * - Main Pass;
 * - Editor Pass.
 *
 * Значение None означает,
 * что текущий draw call не относится
 * ни к одному из перечисленных этапов.
 */
enum class RenderPass {
    /**
     * @brief Render pass не выбран.
     */
    None,

    /**
     * @brief Этап построения shadow map.
     */
    Shadow,

    /**
     * @brief Основной этап рендеринга сцены.
     */
    Main,

    /**
     * @brief Этап отрисовки editor-элементов.
     */
    Editor
};

/**
 * @brief Хранит статистику рендеринга одного кадра.
 *
 * RenderStats используется для накопления
 * количества OpenGL draw calls
 * и времени выполнения отдельных render passes.
 *
 * Статистика разделяется на:
 *
 * - общее количество draw calls;
 * - Shadow Pass;
 * - Main Pass;
 * - Editor Pass.
 *
 * Временные значения хранятся в миллисекундах.
 *
 * Структура содержит только накопленные данные
 * и метод их сброса перед началом нового кадра.
 */
struct RenderStats {
    /**
     * @brief Общее количество draw calls за кадр.
     *
     * Значение увеличивается независимо
     * от текущего RenderPass.
     */
    std::size_t draw_calls = 0;

    /**
     * @brief Количество draw calls,
     * выполненных во время Shadow Pass.
     */
    std::size_t shadow_draw_calls = 0;

    /**
     * @brief Количество draw calls,
     * выполненных во время Main Pass.
     */
    std::size_t main_draw_calls = 0;

    /**
     * @brief Количество draw calls,
     * выполненных во время Editor Pass.
     */
    std::size_t editor_draw_calls = 0;

    /**
     * @brief Суммарное время Shadow Pass.
     *
     * Значение хранится в миллисекундах.
     */
    double shadow_ms = 0.0;

    /**
     * @brief Суммарное время Main Pass.
     *
     * Значение хранится в миллисекундах.
     */
    double main_ms = 0.0;

    /**
     * @brief Суммарное время Editor Pass.
     *
     * Значение хранится в миллисекундах.
     */
    double editor_ms = 0.0;

    /**
     * @brief Сбрасывает всю статистику перед новым кадром.
     *
     * После вызова:
     *
     * - все счётчики draw calls становятся равны 0;
     * - всё накопленное время render passes становится равно 0.0.
     */
    void Reset() {
        draw_calls = 0;

        shadow_draw_calls = 0;
        main_draw_calls = 0;
        editor_draw_calls = 0;

        shadow_ms = 0.0;
        main_ms = 0.0;
        editor_ms = 0.0;
    }
};