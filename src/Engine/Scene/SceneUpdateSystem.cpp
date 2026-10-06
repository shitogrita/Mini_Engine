#include "Engine/Scene/SceneUpdateSystem.h"

void SceneUpdateSystem::Update(Scene& scene, float delta_time, ExecutionMode mode) {
	switch (mode) {
		case ExecutionMode::SingleThreaded:
			UpdateSingleThreaded(scene, delta_time);
			break;

		case ExecutionMode::MultiThreaded:
			UpdateMultiThreaded(scene, delta_time);
			break;
	}
}

void SceneUpdateSystem::UpdateSingleThreaded(Scene& scene, float delta_time) {
	for (const std::shared_ptr<SceneObject>& object : scene.GetObjects()) {
		if (!object) {
			continue;
		}

		MotionState& motion = object->GetMotionState();

		if (!motion.enabled) {
			continue;
		}

		Transform& transform = object->GetTransform();

		transform.position.x += motion.linear_velocity.x * delta_time;
		transform.position.y += motion.linear_velocity.y * delta_time;
		transform.position.z += motion.linear_velocity.z * delta_time;

		transform.rotation.x += motion.angular_velocity.x * delta_time;
		transform.rotation.y += motion.angular_velocity.y * delta_time;
		transform.rotation.z += motion.angular_velocity.z * delta_time;
	}
}

void SceneUpdateSystem::UpdateMultiThreaded(Scene& scene, float delta_time) {
	// Многопоточная реализация будет добавлена после получения baseline.
}