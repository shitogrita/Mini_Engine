#include "Engine/Tests/CubeStressTest.h"

#include "Engine/Scene/MotionState.h"
#include "Engine/Scene/SceneObject.h"
#include "Engine/Scene/Transform.h"

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>

void CubeStressTest::Create(Scene& scene, const std::shared_ptr<Mesh>& cube_mesh, std::size_t object_count) {
    if (!cube_mesh || object_count == 0) {
        return;
    }

    /*
     * Размещаем объекты в двумерной сетке.
     *
     * Например для 1000 объектов:
     *
     * sqrt(1000) ~= 31.6
     *
     * columns = 32
     *
     * В итоге получается примерно:
     *
     * 32 x 32
     *
     * Последняя строка будет заполнена не полностью.
     */
    const std::size_t columns =
        static_cast<std::size_t>(
            std::ceil(
                std::sqrt(
                    static_cast<double>(object_count)
                )
            )
        );

    /*
     * Кубы специально уменьшаются.
     *
     * При десятках тысяч объектов обычный размер 1.0
     * сделал бы тестовую сцену слишком большой.
     */
    constexpr float object_scale = 0.20f;

    /*
     * Расстояние между центрами соседних объектов.
     */
    constexpr float spacing = 0.50f;

    /*
     * Тестовая сетка располагается перед камерой.
     *
     * Предполагается стандартное направление камеры
     * Mini Engine вдоль отрицательной оси Z.
     */
    constexpr float z_position = -20.0f;

    /*
     * Центрируем сетку относительно X/Y = 0.
     */
    const float width =
        static_cast<float>(columns - 1) *
        spacing;

    const float start_x =
        -width * 0.5f;

    const std::size_t rows =
        (object_count + columns - 1) /
        columns;

    const float height =
        static_cast<float>(rows - 1) *
        spacing;

    const float start_y =
        -height * 0.5f;

    for (std::size_t index = 0; index < object_count; ++index) {
        /*
         * Один Mesh используется всеми объектами.
         *
         * Создаётся только новый SceneObject,
         * Transform и MotionState.
         */
        std::shared_ptr<SceneObject> object =
            std::make_shared<SceneObject>(
                "Stress Cube " + std::to_string(index),
                cube_mesh
            );

        object->SetType(SceneObject::Type::Cube);

        /*
         * Преобразуем линейный index в координаты сетки.
         *
         * Например:
         *
         * index = 0  -> row 0, column 0
         * index = 1  -> row 0, column 1
         * ...
         * index = 32 -> row 1, column 0
         */
        const std::size_t row =
            index / columns;

        const std::size_t column =
            index % columns;

        Transform& transform =
            object->GetTransform();

        transform.position =
            Vec3{
                start_x +
                    static_cast<float>(column) *
                    spacing,

                start_y +
                    static_cast<float>(row) *
                    spacing,

                z_position
            };

        transform.rotation =
            Vec3{
                0.0f,
                0.0f,
                0.0f
            };

        transform.scale =
            Vec3{
                object_scale,
                object_scale,
                object_scale
            };

        /*
         * MotionState обязательно включён.
         *
         * Если enabled == false,
         * SceneUpdateSystem сразу пропустит объект,
         * и benchmark почти ничего не будет измерять.
         */
        MotionState& motion =
            object->GetMotionState();

        motion.enabled = true;

        /*
         * Пока не перемещаем кубы в пространстве,
         * чтобы вся тестовая сцена не улетала со временем.
         *
         * Но SceneUpdateSystem всё равно выполняет
         * вычисления linear_velocity * delta_time.
         */
        motion.linear_velocity =
            Vec3{
                0.0f,
                0.0f,
                0.0f
            };

        /*
         * Кубы вращаются с немного разной скоростью.
         *
         * Это даёт реальное изменение Transform
         * на каждом кадре.
         */
        const float speed =
            10.0f +
            static_cast<float>(index % 5) *
            5.0f;

        motion.angular_velocity =
            Vec3{
                speed,
                speed * 0.5f,
                speed * 0.25f
            };

        scene.AddObject(object);
    }
}