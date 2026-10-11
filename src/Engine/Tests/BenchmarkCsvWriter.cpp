#include "Engine/Tests/BenchmarkCsvWriter.h"

#include <chrono>
#include <fstream>
#include <iomanip>

bool BenchmarkCsvWriter::Append(const std::filesystem::path& file_path, std::size_t object_count, ExecutionMode mode, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double fps, std::size_t sample_count) {
    /*
     * Проверяем, существует ли файл и содержит ли он данные.
     *
     * Если файл новый или пустой,
     * перед первой записью добавляется CSV header.
     */
    bool write_header = true;

    if (std::filesystem::exists(file_path)) {
        std::error_code error_code;

        const std::uintmax_t file_size =
            std::filesystem::file_size(
                file_path,
                error_code
            );

        if (!error_code && file_size > 0) {
            write_header = false;
        }
    }

    std::ofstream file(
        file_path,
        std::ios::out | std::ios::app
    );

    if (!file.is_open()) {
        return false;
    }

    if (write_header) {
        file
            << "timestamp,"
            << "objects,"
            << "mode,"
            << "workers,"
            << "update_ms,"
            << "render_ms,"
            << "frame_ms,"
            << "fps,"
            << "samples"
            << '\n';
    }

    /*
     * Получаем время завершения benchmark.
     *
     * Оно позволит позже отличать разные запуски
     * и повторные измерения одной конфигурации.
     */
    const auto now =
        std::chrono::system_clock::now();

    const std::time_t current_time =
        std::chrono::system_clock::to_time_t(now);

    const std::tm* local_time =
        std::localtime(&current_time);

    if (local_time != nullptr) {
        file << std::put_time(
            local_time,
            "%Y-%m-%d %H:%M:%S"
        );
    } else {
        file << "unknown";
    }

    file
        << ','
        << object_count
        << ','
        << ExecutionModeToString(mode)
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
        << fps
        << ','
        << sample_count
        << '\n';

    return file.good();
}

const char* BenchmarkCsvWriter::ExecutionModeToString(ExecutionMode mode) {
    switch (mode) {
        case ExecutionMode::SingleThreaded:
            return "SingleThreaded";

        case ExecutionMode::MultiThreaded:
            return "MultiThreaded";
    }

    return "Unknown";
}