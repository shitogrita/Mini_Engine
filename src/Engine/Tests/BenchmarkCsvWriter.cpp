#include "Engine/Tests/BenchmarkCsvWriter.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <string>

namespace {

/**
 * @brief Преобразует ExecutionMode в строковое представление.
 *
 * Значение используется при записи режима выполнения
 * benchmark в CSV-файл.
 *
 * Возможные результаты:
 *
 * - SingleThreaded;
 * - MultiThreaded;
 * - Unknown.
 *
 * @param execution_mode Режим выполнения benchmark.
 *
 * @return Строковое представление ExecutionMode.
 */
const char* ExecutionModeToString(ExecutionMode execution_mode) {
    switch (execution_mode) {
        case ExecutionMode::SingleThreaded:
            return "SingleThreaded";

        case ExecutionMode::MultiThreaded:
            return "MultiThreaded";
    }

    return "Unknown";
}

/**
 * @brief Создаёт timestamp текущего локального времени.
 *
 * Системное время получается через
 * std::chrono::system_clock,
 * после чего преобразуется в локальное время.
 *
 * Для Windows используется localtime_s(),
 * для остальных платформ — localtime_r().
 *
 * Итоговый формат:
 *
 * YYYY-MM-DD HH:MM:SS
 *
 * @return Строка с текущими локальными датой и временем.
 */
std::string CreateTimestamp() {
    const auto now =
        std::chrono::system_clock::now();

    const std::time_t time =
        std::chrono::system_clock::to_time_t(
            now
        );

    std::tm local_time{};

#if defined(_WIN32)
    localtime_s(
        &local_time,
        &time
    );
#else
    localtime_r(
        &time,
        &local_time
    );
#endif

    char buffer[32]{};

    std::strftime(
        buffer,
        sizeof(buffer),
        "%Y-%m-%d %H:%M:%S",
        &local_time
    );

    return std::string(
        buffer
    );
}

} // namespace

/**
 * @brief Добавляет результат benchmark-запуска в CSV-файл.
 *
 * Метод открывает файл в режиме append,
 * поэтому новые результаты добавляются
 * после уже существующих строк.
 *
 * Если файл:
 *
 * - ещё не существует;
 * - существует, но имеет размер 0,
 *
 * перед первой записью добавляется CSV-header.
 *
 * В CSV сохраняются:
 *
 * - timestamp;
 * - номер запуска;
 * - количество объектов;
 * - режим выполнения;
 * - количество worker-потоков;
 * - время CPU update;
 * - время render;
 * - время обработки кадра;
 * - processing FPS;
 * - время Shadow Pass;
 * - время Main Pass;
 * - время Editor Pass;
 * - общее количество draw calls;
 * - draw calls Shadow Pass;
 * - draw calls Main Pass;
 * - draw calls Editor Pass;
 * - количество samples.
 *
 * Числовые значения времени и FPS
 * записываются через std::fixed
 * с точностью 4 знака после запятой.
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
 * @return true, если после записи поток находится
 * в корректном состоянии.
 * @return false, если файл не удалось открыть
 * или во время записи произошла ошибка потока.
 */
bool BenchmarkCsvWriter::Append(const std::filesystem::path& path, std::size_t run_index, std::size_t object_count, ExecutionMode execution_mode, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double processing_fps, double shadow_ms, double main_ms, double editor_ms, std::size_t draw_calls, std::size_t shadow_draw_calls, std::size_t main_draw_calls, std::size_t editor_draw_calls, std::size_t sample_count) {
    std::error_code error_code;

    /**
     * Проверяем существование файла,
     * не выбрасывая filesystem exception.
     */
    const bool file_exists =
        std::filesystem::exists(
            path,
            error_code
        );

    /**
     * Для нового файла необходимо записать header.
     */
    bool write_header =
        !file_exists;

    /**
     * Если файл существует,
     * дополнительно проверяем его размер.
     *
     * Пустой существующий файл также должен
     * получить строку с названиями колонок.
     */
    if (
        file_exists &&
        !error_code
    ) {
        const std::uintmax_t file_size =
            std::filesystem::file_size(
                path,
                error_code
            );

        if (
            !error_code &&
            file_size == 0
        ) {
            write_header = true;
        }
    }

    /**
     * Открываем файл в append mode,
     * чтобы не удалять результаты
     * предыдущих benchmark-запусков.
     */
    std::ofstream file(
        path,
        std::ios::app
    );

    if (!file.is_open()) {
        return false;
    }

    /**
     * Записываем названия колонок
     * только для нового или пустого файла.
     */
    if (write_header) {
        file
            << "timestamp,"
            << "run,"
            << "objects,"
            << "mode,"
            << "workers,"
            << "update_ms,"
            << "render_ms,"
            << "frame_ms,"
            << "processing_fps,"
            << "shadow_ms,"
            << "main_ms,"
            << "editor_ms,"
            << "draw_calls,"
            << "shadow_draw_calls,"
            << "main_draw_calls,"
            << "editor_draw_calls,"
            << "samples"
            << '\n';
    }

    /**
     * Записываем одну строку benchmark-результата.
     *
     * Порядок значений должен соответствовать
     * порядку колонок CSV-header.
     */
    file
        << CreateTimestamp()
        << ','
        << run_index
        << ','
        << object_count
        << ','
        << ExecutionModeToString(
            execution_mode
        )
        << ','
        << worker_count
        << ','
        << std::fixed
        << std::setprecision(4)
        << update_ms
        << ','
        << render_ms
        << ','
        << frame_ms
        << ','
        << processing_fps
        << ','
        << shadow_ms
        << ','
        << main_ms
        << ','
        << editor_ms
        << ','
        << draw_calls
        << ','
        << shadow_draw_calls
        << ','
        << main_draw_calls
        << ','
        << editor_draw_calls
        << ','
        << sample_count
        << '\n';

    /**
     * good() позволяет проверить,
     * что поток остался в корректном состоянии
     * после выполнения записи.
     */
    return file.good();
}