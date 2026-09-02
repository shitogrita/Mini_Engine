#pragma once

#include "Engine/Renderer/Mesh.h"
#include "Engine/Scene/BoundingBox.h"
#include "Engine/Scene/Transform.h"

#include <memory>
#include <string>
#include <utility>


/**
 * @brief Представляет отдельный объект, находящийся в сцене.
 *
 * SceneObject объединяет данные, которые относятся
 * к одному объекту сцены:
 *
 * - имя объекта;
 * - Transform — положение, поворот и масштаб;
 * - Mesh — геометрию, которую необходимо отрисовать;
 * - BoundingBox — границы геометрии объекта.
 *
 * Сам SceneObject не занимается отрисовкой.
 * Отрисовка выполняется Renderer.
 *
 * SceneObject также не хранит список других объектов.
 * Несколько SceneObject находятся внутри Scene.
 */
class SceneObject {
public:
    /**
     * @brief Создаёт объект сцены со значениями по умолчанию.
     */
    SceneObject() = default;


    /**
     * @brief Создаёт объект сцены с указанным именем.
     *
     * @param name Имя объекта.
     */
    explicit SceneObject(
        std::string name
    );


    /**
     * @brief Создаёт объект сцены с именем и готовым Mesh.
     *
     * @param name Имя объекта.
     * @param mesh Геометрия объекта.
     */
    SceneObject(
        std::string name,
        std::shared_ptr<Mesh> mesh
    );


    /**
     * @brief Возвращает имя объекта.
     *
     * @return Имя SceneObject.
     */
    const std::string& GetName() const;


    /**
     * @brief Изменяет имя объекта.
     *
     * @param name Новое имя.
     */
    void SetName(
        std::string name
    );


    /**
     * @brief Возвращает изменяемый Transform объекта.
     *
     * Этот вариант используется, когда необходимо изменить
     * position, rotation или scale.
     *
     * @return Ссылка на Transform объекта.
     */
    Transform& GetTransform();


    /**
     * @brief Возвращает Transform объекта только для чтения.
     *
     * @return Константная ссылка на Transform объекта.
     */
    const Transform& GetTransform() const;


    /**
     * @brief Устанавливает геометрию объекта.
     *
     * Используется shared_ptr, потому что в дальнейшем
     * несколько SceneObject смогут ссылаться на один и тот же Mesh.
     *
     * Это позволит не создавать одинаковую геометрию на GPU
     * для каждого экземпляра одного объекта.
     *
     * @param mesh Геометрия объекта.
     */
    void SetMesh(
        std::shared_ptr<Mesh> mesh
    );


    /**
     * @brief Возвращает Mesh объекта.
     *
     * @return shared_ptr на Mesh.
     */
    std::shared_ptr<Mesh> GetMesh();


    /**
     * @brief Возвращает Mesh только для чтения.
     *
     * @return shared_ptr на константный Mesh.
     */
    std::shared_ptr<const Mesh> GetMesh() const;


    /**
     * @brief Проверяет, содержит ли SceneObject геометрию.
     *
     * @return true, если Mesh существует.
     */
    bool HasMesh() const;


    /**
     * @brief Устанавливает BoundingBox объекта.
     *
     * Сейчас BoundingBox описывает исходную геометрию Mesh
     * в локальных координатах модели.
     *
     * То есть position, rotation и scale объекта
     * ещё не применены к этой коробке.
     *
     * @param bounding_box Границы геометрии объекта.
     */
    void SetBoundingBox(
        const BoundingBox& bounding_box
    );


    /**
     * @brief Возвращает BoundingBox объекта.
     *
     * В дальнейшем этот BoundingBox будет использоваться
     * при выборе объектов мышью через проверку пересечения луча.
     *
     * @return BoundingBox объекта.
     */
    const BoundingBox& GetBoundingBox() const;


private:
    /**
     * @brief Имя объекта, отображаемое в Hierarchy.
     */
    std::string name_{
        "SceneObject"
    };


    /**
     * @brief Пространственное преобразование объекта.
     *
     * Содержит:
     * - position;
     * - rotation;
     * - scale.
     */
    Transform transform_{};


    /*
     * Геометрия, связанная с объектом.
     *
     * Используется shared_ptr, потому что в дальнейшем
     * несколько SceneObject смогут использовать один Mesh.
     *
     * Например:
     *
     * Cube_1 ----\
     *             -> один Mesh куба
     * Cube_2 ----/
     */
    std::shared_ptr<Mesh> mesh_;


    /*
     * Ограничивающая коробка исходной геометрии.
     *
     * BoundingBox пока хранится в локальных координатах модели.
     *
     * Позже он будет использоваться для:
     * - выбора объекта мышью;
     * - проверки попадания луча;
     * - пространственных оптимизаций.
     */
    BoundingBox bounding_box_{};
};