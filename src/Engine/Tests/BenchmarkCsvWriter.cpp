#include "Engine/Tests/BenchmarkCsvWriter.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <string>

namespace {

const char* ExecutionModeToString(ExecutionMode execution_mode) {
    switch (execution_mode) {
        case ExecutionMode::SingleThreaded:
            return "SingleThreaded";

        case ExecutionMode::MultiThreaded:
            return "MultiThreaded";
    }

    return "Unknown";
}

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

}

bool BenchmarkCsvWriter::Append(const std::filesystem::path& path, std::size_t run_index, std::size_t object_count, ExecutionMode execution_mode, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double processing_fps, double shadow_ms, double main_ms, double editor_ms, std::size_t draw_calls, std::size_t shadow_draw_calls, std::size_t main_draw_calls, std::size_t editor_draw_calls, std::size_t sample_count) {
    std::error_code error_code;

    const bool file_exists =
        std::filesystem::exists(
            path,
            error_code
        );

    bool write_header =
        !file_exists;

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

    std::ofstream file(
        path,
        std::ios::app
    );

    if (!file.is_open()) {
        return false;
    }

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

    return file.good();
}