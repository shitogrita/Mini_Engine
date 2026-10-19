#pragma once

#include "Engine/Core/ExecutionMode.h"

#include <cstddef>
#include <filesystem>

class BenchmarkCsvWriter {
public:
	static bool Append(const std::filesystem::path& path, std::size_t run_index, std::size_t object_count, ExecutionMode execution_mode, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double processing_fps, double shadow_ms, double main_ms, double editor_ms, std::size_t draw_calls, std::size_t shadow_draw_calls, std::size_t main_draw_calls, std::size_t editor_draw_calls, std::size_t sample_count);
};