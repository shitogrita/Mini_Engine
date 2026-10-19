#pragma once

#include <cstddef>

/**
 * @brief Тип текущего этапа рендеринга.
 */
enum class RenderPass {
	None,
	Shadow,
	Main,
	Editor
};

/**
 * @brief Статистика рендеринга одного кадра.
 *
 * Содержит количество draw calls и время,
 * затраченное на отдельные этапы рендеринга.
 */
struct RenderStats {
	std::size_t draw_calls = 0;

	std::size_t shadow_draw_calls = 0;
	std::size_t main_draw_calls = 0;
	std::size_t editor_draw_calls = 0;

	double shadow_ms = 0.0;
	double main_ms = 0.0;
	double editor_ms = 0.0;

	/**
	 * @brief Сбрасывает статистику перед новым кадром.
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