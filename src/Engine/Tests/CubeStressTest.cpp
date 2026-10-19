#include "Engine/Tests/CubeStressTest.h"

#include "Engine/Scene/MotionState.h"
#include "Engine/Scene/SceneObject.h"
#include "Engine/Scene/Transform.h"

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>

/**
 * @brief Создаёт масштабируемую нагрузочную сцену из Cube.
 *
 * Количество создаваемых объектов задаётся параметром object_count.
 *
 * Все SceneObject используют один общий cube_mesh,
 * но имеют собственные:
 *
 * - Transform;
 * - MotionState;
 * - имя.
 *
 * Объекты размещаются в двумерной сетке
 * в плоскости XY на фиксированной координате Z.
 *
 * Количество столбцов вычисляется как:
 *
 * ceil(sqrt(object_count)).
 *
 * Это формирует сетку, максимально близкую
 * по пропорциям к квадратной.
 *
 * Для всех объектов включается MotionState.
 * Линейная скорость равна нулю,
 * а угловая скорость изменяется
 * в зависимости от индекса объекта.
 *
 * @param scene Сцена, в которую добавляются тестовые объекты.
 * @param cube_mesh Общий Mesh, используемый всеми Cube.
 * @param object_count Количество SceneObject,
 * которое необходимо создать.
 */
void CubeStressTest::Create(Scene& scene, const std::shared_ptr<Mesh>& cube_mesh, std::size_t object_count) {
    /**
     * Без Mesh или при нулевом количестве объектов
     * тестовая сцена не создаётся.
     */
    if (!cube_mesh || object_count == 0) {
        return;
    }

    /**
     * Вычисляем количество столбцов двумерной сетки.
     *
     * Используется округление вверх:
     *
     * columns = ceil(sqrt(object_count)).
     *
     * Например:
     *
     * object_count = 1000
     *
     * sqrt(1000) ~= 31.6
     *
     * columns = 32.
     *
     * Последняя строка сетки
     * при необходимости остаётся заполненной частично.
     */
    const std::size_t columns =
        static_cast<std::size_t>(
            std::ceil(
                std::sqrt(
                    static_cast<double>(object_count)
                )
            )
        );

    /**
     * Масштаб каждого тестового Cube.
     *
     * Объекты намеренно уменьшаются,
     * чтобы сцена с большим количеством Cube
     * оставалась компактной.
     */
    constexpr float object_scale = 0.20f;

    /**
     * Расстояние между центрами
     * соседних объектов сетки.
     */
    constexpr float spacing = 0.50f;

    /**
     * Общая координата Z для всей тестовой сетки.
     *
     * Сетка располагается перед камерой
     * при стандартном направлении взгляда
     * Mini Engine вдоль отрицательной оси Z.
     */
    constexpr float z_position = -20.0f;

    /**
     * Вычисляем горизонтальный размер сетки,
     * чтобы затем расположить её центр
     * относительно X = 0.
     */
    const float width =
        static_cast<float>(columns - 1) *
        spacing;

    const float start_x =
        -width * 0.5f;

    /**
     * Вычисляем необходимое количество строк.
     *
     * Используется целочисленное округление вверх:
     *
     * rows = ceil(object_count / columns).
     */
    const std::size_t rows =
        (object_count + columns - 1) /
        columns;

    /**
     * Аналогично X вычисляем вертикальный размер
     * и центрируем сетку относительно Y = 0.
     */
    const float height =
        static_cast<float>(rows - 1) *
        spacing;

    const float start_y =
        -height * 0.5f;

    /**
     * Создаём заданное количество SceneObject.
     */
    for (std::size_t index = 0; index < object_count; ++index) {
        /**
         * Один Mesh совместно используется
         * всеми тестовыми объектами.
         *
         * Для каждого Cube создаётся только
         * новый SceneObject и его CPU-side состояние.
         */
        std::shared_ptr<SceneObject> object =
            std::make_shared<SceneObject>(
                "Stress Cube " + std::to_string(index),
                cube_mesh
            );

        object->SetType(SceneObject::Type::Cube);

        /**
         * Преобразуем линейный индекс объекта
         * в строку и столбец двумерной сетки.
         *
         * row = index / columns
         *
         * column = index % columns
         */
        const std::size_t row =
            index / columns;

        const std::size_t column =
            index % columns;

        Transform& transform =
            object->GetTransform();

        /**
         * Размещаем Cube в соответствующей
         * ячейке сетки.
         */
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

        /**
         * Начальное вращение отсутствует.
         */
        transform.rotation =
            Vec3{
                0.0f,
                0.0f,
                0.0f
            };

        /**
         * Используем одинаковый уменьшенный масштаб
         * по всем трём координатным осям.
         */
        transform.scale =
            Vec3{
                object_scale,
                object_scale,
                object_scale
            };

        MotionState& motion =
            object->GetMotionState();

        /**
         * MotionState обязательно включён,
         * чтобы SceneUpdateSystem обрабатывал объект
         * при каждом CPU update.
         */
        motion.enabled = true;

        /**
         * Линейная скорость равна нулю.
         *
         * Поэтому Cube остаются
         * в своих исходных позициях.
         *
         * SceneUpdateSystem при этом по-прежнему
         * выполняет вычисления обновления position.
         */
        motion.linear_velocity =
            Vec3{
                0.0f,
                0.0f,
                0.0f
            };

        /**
         * Выбираем одну из пяти скоростей вращения
         * в зависимости от индекса объекта:
         *
         * 10, 15, 20, 25 или 30.
         */
        const float speed =
            10.0f +
            static_cast<float>(index % 5) *
            5.0f;

        /**
         * Кубы вращаются одновременно
         * вокруг всех трёх осей.
         *
         * Скорость Y составляет половину speed,
         * а скорость Z — четверть speed.
         */
        motion.angular_velocity =
            Vec3{
                speed,
                speed * 0.5f,
                speed * 0.25f
            };

        /**
         * Добавляем готовый объект
         * в тестовую Scene.
         */
        scene.AddObject(object);
    }
}