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

/**
 * @brief Количество SceneObject,
 * используемых в correctness test.
 */
constexpr std::size_t kObjectCount = 10'000;

/**
 * @brief Количество последовательных кадров,
 * выполняемых в каждом режиме.
 */
constexpr int kFrameCount = 300;

/**
 * @brief Фиксированный delta time одного кадра.
 *
 * Значение соответствует приблизительно 60 обновлениям в секунду.
 */
constexpr float kDeltaTime = 1.0f / 60.0f;

/**
 * @brief Допустимая абсолютная погрешность
 * при сравнении float-значений.
 */
constexpr float kEpsilon = 0.00001f;

/**
 * @brief Сравнивает два float-значения
 * с заданной абсолютной погрешностью.
 *
 * Значения считаются равными,
 * если абсолютная разница между ними
 * не превышает kEpsilon.
 *
 * @param first Первое значение.
 * @param second Второе значение.
 *
 * @return true, если значения достаточно близки,
 * иначе false.
 */
bool NearlyEqual(float first, float second) {
    return std::abs(first - second) <= kEpsilon;
}

/**
 * @brief Сравнивает два Vec3
 * с учётом погрешности float.
 *
 * Каждая компонента X, Y и Z
 * сравнивается через NearlyEqual().
 *
 * @param first Первый вектор.
 * @param second Второй вектор.
 *
 * @return true, если все три компоненты совпадают
 * с точностью kEpsilon.
 */
bool CompareVec3(const Vec3& first, const Vec3& second) {
    return
        NearlyEqual(first.x, second.x) &&
        NearlyEqual(first.y, second.y) &&
        NearlyEqual(first.z, second.z);
}

/**
 * @brief Заполняет Scene фиксированным набором объектов.
 *
 * Создаётся kObjectCount независимых SceneObject.
 *
 * Для каждого объекта задаются:
 *
 * - уникальное имя;
 * - начальная position;
 * - начальная rotation;
 * - MotionState;
 * - linear_velocity;
 * - angular_velocity.
 *
 * Параметры вычисляются детерминированно
 * на основе индекса объекта.
 *
 * Благодаря этому повторный вызов FillScene()
 * создаёт сцену с теми же исходными данными,
 * но с отдельными SceneObject.
 *
 * Часть объектов получает MotionState::enabled = false,
 * что дополнительно проверяет одинаковое поведение
 * пропуска неактивных объектов в обоих режимах.
 *
 * @param scene Сцена, которую необходимо заполнить.
 */
void FillScene(Scene& scene) {
    for (std::size_t index = 0; index < kObjectCount; ++index) {
        std::shared_ptr<SceneObject> object =
            std::make_shared<SceneObject>(
                "Correctness Object " +
                std::to_string(index)
            );

        Transform& transform =
            object->GetTransform();

        /**
         * Начальная позиция зависит от индекса,
         * чтобы тестовые объекты не имели
         * полностью одинакового состояния.
         */
        transform.position = Vec3{
            static_cast<float>(index % 100) * 0.1f,
            static_cast<float>((index / 100) % 100) * 0.1f,
            static_cast<float>(index % 17) * 0.05f
        };

        /**
         * Начальное вращение также различается
         * между объектами.
         */
        transform.rotation = Vec3{
            static_cast<float>(index % 7),
            static_cast<float>(index % 11),
            static_cast<float>(index % 13)
        };

        MotionState& motion =
            object->GetMotionState();

        /**
         * Каждый одиннадцатый объект
         * получает отключённый MotionState.
         *
         * SceneUpdateSystem должен одинаково
         * пропустить его в обоих режимах.
         */
        motion.enabled =
            index % 11 != 0;

        /**
         * Линейная скорость немного различается
         * между объектами.
         */
        motion.linear_velocity = Vec3{
            0.10f + static_cast<float>(index % 5) * 0.01f,
            -0.05f + static_cast<float>(index % 3) * 0.01f,
            0.02f + static_cast<float>(index % 7) * 0.005f
        };

        /**
         * Угловая скорость также зависит
         * от индекса SceneObject.
         */
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

/**
 * @brief Сравнивает результаты обновления двух Scene.
 *
 * Сначала проверяется одинаковое количество объектов.
 *
 * Затем объекты сравниваются попарно
 * по одинаковым индексам.
 *
 * Для каждого SceneObject проверяются:
 *
 * - position;
 * - rotation;
 * - scale.
 *
 * Сравнение Vec3 выполняется через CompareVec3()
 * с абсолютной погрешностью kEpsilon.
 *
 * Scale в текущем SceneUpdateSystem
 * непосредственно не изменяется,
 * однако также проверяется,
 * чтобы убедиться в отсутствии нежелательных изменений
 * других компонентов Transform.
 *
 * @param single_scene Сцена после SingleThreaded update.
 * @param multi_scene Сцена после MultiThreaded update.
 *
 * @return true, если результаты обновления сцен совпадают.
 * @return false при первом обнаруженном различии.
 */
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

        /**
         * Если один из указателей пуст,
         * сцены считаются одинаковыми в этой позиции
         * только при совпадении обоих shared_ptr.
         */
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
         * но дополнительно проверяем,
         * что MultiThreaded update
         * не затронул эту часть Transform.
         */
        if (!CompareVec3(single_transform.scale, multi_transform.scale)) {
            return false;
        }
    }

    return true;
}

} // namespace

/**
 * @brief Проверяет функциональную эквивалентность
 * SingleThreaded и MultiThreaded Scene Update.
 *
 * Тест создаёт две независимые Scene
 * с одинаковыми начальными данными.
 *
 * Первая сцена обновляется через:
 *
 * ExecutionMode::SingleThreaded.
 *
 * Вторая сцена обновляется через:
 *
 * ExecutionMode::MultiThreaded.
 *
 * Для MultiThreaded режима явно устанавливаются
 * четыре worker-потока.
 *
 * Обе сцены проходят kFrameCount последовательных обновлений
 * с одинаковым фиксированным delta time.
 *
 * После этого итоговые Transform
 * попарно сравниваются через CompareScenes().
 *
 * Использование независимых SceneObject важно:
 * совместное использование одних и тех же shared_ptr
 * привело бы к изменению одинаковых объектов
 * двумя SceneUpdateSystem и сделало бы тест некорректным.
 *
 * @return true, если SingleThreaded и MultiThreaded
 * привели к одинаковому итоговому состоянию Scene.
 * @return false, если обнаружено различие.
 */
bool SceneUpdateCorrectnessTest::Run() {
    Scene single_scene;
    Scene multi_scene;

    /**
     * Создаём две независимые,
     * но детерминированно одинаковые Scene.
     */
    FillScene(single_scene);
    FillScene(multi_scene);

    SceneUpdateSystem single_update_system;
    SceneUpdateSystem multi_update_system;

    /**
     * Несколько worker-потоков заставляют
     * MultiThreaded режим реально разделить
     * объекты между несколькими jobs.
     */
    multi_update_system.SetWorkerCount(4);

    /**
     * Выполняем 300 последовательных обновлений.
     *
     * Это позволяет проверить не только
     * один вызов Update(),
     * но и накопление изменений Transform
     * за продолжительный период обновления.
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