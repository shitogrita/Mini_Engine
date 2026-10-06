#pragma once

#include "Engine/Math/matrix_types.h"

/**
 * @brief Параметры движения SceneObject.
 *
 * MotionState хранит линейную и угловую скорость объекта.
 */
struct MotionState {
	Vec3 linear_velocity{0.0f, 0.0f, 0.0f};
	Vec3 angular_velocity{0.0f, 0.0f, 0.0f};

	bool enabled = false;
};