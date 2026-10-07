#pragma once

#include <cstddef>

struct PerformanceStats {
	std::size_t object_count = 0;
	std::size_t worker_count = 1;

	double update_ms = 0.0;
	double render_ms = 0.0;
	double frame_ms = 0.0;
	double fps = 0.0;
};