#pragma once

#include "Engine/Core/ExecutionMode.h"

#include <cstddef>
#include <filesystem>

/**
 * @brief Выполняет запись результатов benchmark в CSV-файл.
 *
 * BenchmarkCsvWriter используется для сохранения
 * результатов тестирования производительности Mini Engine.
 *
 * В файл записываются:
 *
 * - timestamp запуска;
 * - номер запуска;
 * - количество SceneObject;
 * - режим выполнения;
 * - количество worker-потоков;
 * - время CPU update;
 * - время render;
 * - время обработки кадра;
 * - processing FPS;
 * - время отдельных render passes;
 * - количество draw calls;
 * - количество samples.
 *
 * Новые результаты добавляются
 * в существующий CSV-файл без удаления
 * предыдущих benchmark-запусков.
 */
class BenchmarkCsvWriter {
public:
    /**
     * @brief Добавляет результат benchmark-запуска в CSV-файл.
     *
     * Если файл отсутствует или имеет нулевой размер,
     * перед первой строкой данных записывается CSV-header.
     *
     * Запись выполняется в режиме append,
     * поэтому существующие результаты сохраняются.
     *
     * Значения update_ms, render_ms, frame_ms,
     * processing_fps и времени render passes
     * записываются с фиксированной точностью.
     *
     * @param path Путь к CSV-файлу.
     * @param run_index Номер benchmark-запуска.
     * @param object_count Количество объектов сцены.
     * @param execution_mode Режим выполнения Scene Update.
     * @param worker_count Количество worker-потоков.
     * @param update_ms Время CPU Scene Update в миллисекундах.
     * @param render_ms Время рендеринга в миллисекундах.
     * @param frame_ms Время обработки кадра в миллисекундах.
     * @param processing_fps Производительность обработки кадров.
     * @param shadow_ms Время Shadow Pass в миллисекундах.
     * @param main_ms Время Main Pass в миллисекундах.
     * @param editor_ms Время Editor Pass в миллисекундах.
     * @param draw_calls Общее количество draw calls.
     * @param shadow_draw_calls Количество draw calls Shadow Pass.
     * @param main_draw_calls Количество draw calls Main Pass.
     * @param editor_draw_calls Количество draw calls Editor Pass.
     * @param sample_count Количество измерений, использованных для результата.
     *
     * @return true, если запись завершилась успешно.
     * @return false, если файл невозможно открыть
     * или поток записи перешёл в ошибочное состояние.
     */
    static bool Append(const std::filesystem::path& path, std::size_t run_index, std::size_t object_count, ExecutionMode execution_mode, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double processing_fps, double shadow_ms, double main_ms, double editor_ms, std::size_t draw_calls, std::size_t shadow_draw_calls, std::size_t main_draw_calls, std::size_t editor_draw_calls, std::size_t sample_count);
};