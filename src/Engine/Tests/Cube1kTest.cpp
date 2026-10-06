#include "Cube1kTest.h"

#include "Engine/Scene/MotionState.h"
#include "Engine/Scene/SceneObject.h"

#include <memory>
#include <string>

/**
 * @brief Создаёт сцену из 1000 Cube.
 *
 * Кубы располагаются сеткой 10 x 10 x 10.
 * Все объекты используют один общий Mesh.
 */
void Cube1kTest::Create(Scene& scene, const std::shared_ptr<Mesh>& cube_mesh) {
	if (!cube_mesh) {
		return;
	}

	constexpr int count_x = 10;
	constexpr int count_y = 10;
	constexpr int count_z = 10;

	constexpr float spacing = 2.0f;

	int object_index = 0;

	for (int x = 0; x < count_x; ++x) {
		for (int y = 0; y < count_y; ++y) {
			for (int z = 0; z < count_z; ++z) {
				std::shared_ptr<SceneObject> object = std::make_shared<SceneObject>(
					"Cube_" + std::to_string(object_index),
					cube_mesh
				);

				Transform& transform = object->GetTransform();

				transform.position = Vec3{
					static_cast<float>(x) * spacing,
					static_cast<float>(y) * spacing,
					static_cast<float>(z) * spacing
				};

				MotionState& motion = object->GetMotionState();

				motion.enabled = true;

				motion.linear_velocity = Vec3{
					0.0f,
					0.0f,
					0.0f
				};

				motion.angular_velocity = Vec3{
					15.0f + static_cast<float>(x),
					20.0f + static_cast<float>(y),
					25.0f + static_cast<float>(z)
				};

				object->SetType(SceneObject::Type::Cube);

				scene.AddObject(object);

				++object_index;
			}
		}
	}
}