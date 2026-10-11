#pragma once

#include "Engine/Core/ExecutionMode.h"

#include <cstddef>
#include <filesystem>

class BenchmarkCsvWriter {
public:
	static bool Append(const std::filesystem::path& file_path, std::size_t object_count, ExecutionMode mode, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double fps, std::size_t sample_count);

private:
	static const char* ExecutionModeToString(ExecutionMode mode);
};