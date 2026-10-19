#include "Cube1kTest.h"

#include "Engine/Scene/MotionState.h"
#include "Engine/Scene/SceneObject.h"

#include <memory>
#include <string>

/**
 * @brief Создаёт тестовую сцену из 1000 Cube.
 *
 * Объекты размещаются в трёхмерной сетке:
 *
 * 10 x 10 x 10.
 *
 * Общее количество объектов:
 *
 * 10 * 10 * 10 = 1000.
 *
 * Все SceneObject используют один общий Mesh,
 * переданный через cube_mesh.
 *
 * Это позволяет не создавать отдельную геометрию
 * для каждого тестового объекта и использовать
 * одну и ту же GPU-side геометрию совместно.
 *
 * Каждый Cube получает:
 *
 * - уникальное имя;
 * - собственный Transform;
 * - включённый MotionState;
 * - нулевую линейную скорость;
 * - угловую скорость, зависящую от положения в сетке;
 * - тип SceneObject::Type::Cube.
 *
 * @param scene Сцена, в которую добавляются тестовые объекты.
 * @param cube_mesh Общий Mesh, используемый всеми Cube.
 */
void Cube1kTest::Create(Scene& scene, const std::shared_ptr<Mesh>& cube_mesh) {
    /**
     * Без корректного Mesh создание тестовой сцены
     * не выполняется.
     */
    if (!cube_mesh) {
        return;
    }

    /**
     * Количество объектов по каждой координатной оси.
     *
     * Итоговое количество:
     *
     * 10 * 10 * 10 = 1000.
     */
    constexpr int count_x = 10;
    constexpr int count_y = 10;
    constexpr int count_z = 10;

    /**
     * Расстояние между соседними объектами сетки.
     */
    constexpr float spacing = 2.0f;

    /**
     * Последовательный индекс используется
     * для формирования уникального имени Cube.
     */
    int object_index = 0;

    /**
     * Формируем трёхмерную сетку объектов.
     */
    for (int x = 0; x < count_x; ++x) {
        for (int y = 0; y < count_y; ++y) {
            for (int z = 0; z < count_z; ++z) {
                /**
                 * Все объекты получают один и тот же shared_ptr<Mesh>.
                 *
                 * Геометрия Cube таким образом разделяется
                 * между всеми SceneObject.
                 */
                std::shared_ptr<SceneObject> object = std::make_shared<SceneObject>(
                    "Cube_" + std::to_string(object_index),
                    cube_mesh
                );

                Transform& transform = object->GetTransform();

                /**
                 * Позиция объекта определяется индексами
                 * его ячейки в сетке и значением spacing.
                 */
                transform.position = Vec3{
                    static_cast<float>(x) * spacing,
                    static_cast<float>(y) * spacing,
                    static_cast<float>(z) * spacing
                };

                MotionState& motion = object->GetMotionState();

                /**
                 * Включаем обновление MotionState,
                 * чтобы объект участвовал в CPU Scene Update.
                 */
                motion.enabled = true;

                /**
                 * Линейное движение отсутствует.
                 *
                 * Объекты остаются в своих позициях,
                 * изменяется только их rotation.
                 */
                motion.linear_velocity = Vec3{
                    0.0f,
                    0.0f,
                    0.0f
                };

                /**
                 * Угловая скорость зависит
                 * от координат объекта в сетке.
                 *
                 * Это даёт объектам разные параметры вращения,
                 * сохраняя одинаковую вычислительную схему обновления.
                 */
                motion.angular_velocity = Vec3{
                    15.0f + static_cast<float>(x),
                    20.0f + static_cast<float>(y),
                    25.0f + static_cast<float>(z)
                };

                object->SetType(SceneObject::Type::Cube);

                /**
                 * Добавляем готовый SceneObject
                 * в тестовую Scene.
                 */
                scene.AddObject(object);

                ++object_index;
            }
        }
    }
}