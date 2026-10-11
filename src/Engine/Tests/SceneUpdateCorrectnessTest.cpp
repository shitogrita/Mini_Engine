#include "Engine/Tests/SceneUpdateCorrectnessTest.h"

#include "Engine/Core/ExecutionMode.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneObject.h"
#include "Engine/Scene/SceneUpdateSystem.h"

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>

namespace {

constexpr std::size_t kObjectCount = 10'000;
constexpr int kFrameCount = 300;
constexpr float kDeltaTime = 1.0f / 60.0f;
constexpr float kEpsilon = 0.00001f;

bool NearlyEqual(float first, float second) {
    return std::abs(first - second) <= kEpsilon;
}

bool CompareVec3(const Vec3& first, const Vec3& second) {
    return
        NearlyEqual(first.x, second.x) &&
        NearlyEqual(first.y, second.y) &&
        NearlyEqual(first.z, second.z);
}

void FillScene(Scene& scene) {
    for (std::size_t index = 0; index < kObjectCount; ++index) {
        std::shared_ptr<SceneObject> object =
            std::make_shared<SceneObject>(
                "Correctness Object " +
                std::to_string(index)
            );

        Transform& transform =
            object->GetTransform();

        transform.position = Vec3{
            static_cast<float>(index % 100) * 0.1f,
            static_cast<float>((index / 100) % 100) * 0.1f,
            static_cast<float>(index % 17) * 0.05f
        };

        transform.rotation = Vec3{
            static_cast<float>(index % 7),
            static_cast<float>(index % 11),
            static_cast<float>(index % 13)
        };

        MotionState& motion =
            object->GetMotionState();

        motion.enabled =
            index % 11 != 0;

        motion.linear_velocity = Vec3{
            0.10f + static_cast<float>(index % 5) * 0.01f,
            -0.05f + static_cast<float>(index % 3) * 0.01f,
            0.02f + static_cast<float>(index % 7) * 0.005f
        };

        motion.angular_velocity = Vec3{
            1.0f + static_cast<float>(index % 4) * 0.1f,
            0.5f + static_cast<float>(index % 6) * 0.1f,
            -0.25f + static_cast<float>(index % 3) * 0.05f
        };

        scene.AddObject(
            object
        );
    }
}

bool CompareScenes(const Scene& single_scene, const Scene& multi_scene) {
    const auto& single_objects =
        single_scene.GetObjects();

    const auto& multi_objects =
        multi_scene.GetObjects();

    if (single_objects.size() != multi_objects.size()) {
        return false;
    }

    for (std::size_t index = 0; index < single_objects.size(); ++index) {
        const std::shared_ptr<SceneObject>& single_object =
            single_objects[index];

        const std::shared_ptr<SceneObject>& multi_object =
            multi_objects[index];

        if (!single_object || !multi_object) {
            if (single_object != multi_object) {
                return false;
            }

            continue;
        }

        const Transform& single_transform =
            single_object->GetTransform();

        const Transform& multi_transform =
            multi_object->GetTransform();

        if (!CompareVec3(single_transform.position, multi_transform.position)) {
            return false;
        }

        if (!CompareVec3(single_transform.rotation, multi_transform.rotation)) {
            return false;
        }

        /**
         * Scale в UpdateObject() сейчас не изменяется,
         * но дополнительно проверяем, что MultiThreaded
         * случайно не затронул остальные данные Transform.
         */
        if (!CompareVec3(single_transform.scale, multi_transform.scale)) {
            return false;
        }
    }

    return true;
}

}

bool SceneUpdateCorrectnessTest::Run() {
    Scene single_scene;
    Scene multi_scene;

    /**
     * Создаём две независимые, но полностью
     * одинаковые Scene.
     *
     * Нельзя использовать одни и те же shared_ptr,
     * иначе Single и Multi изменяли бы одни объекты.
     */
    FillScene(single_scene);
    FillScene(multi_scene);

    SceneUpdateSystem single_update_system;
    SceneUpdateSystem multi_update_system;

    /**
     * Четырёх workers достаточно, чтобы проверить
     * реальное разбиение Scene на несколько задач.
     */
    multi_update_system.SetWorkerCount(4);

    /**
     * Прогоняем не один, а 300 кадров.
     *
     * Так тест проверяет не только единичный вызов,
     * но и накопление изменений во времени.
     */
    for (int frame = 0; frame < kFrameCount; ++frame) {
        single_update_system.Update(
            single_scene,
            kDeltaTime,
            ExecutionMode::SingleThreaded
        );

        multi_update_system.Update(
            multi_scene,
            kDeltaTime,
            ExecutionMode::MultiThreaded
        );
    }

    return CompareScenes(
        single_scene,
        multi_scene
    );
}