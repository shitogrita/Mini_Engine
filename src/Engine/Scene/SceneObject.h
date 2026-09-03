#pragma once

#include "Engine/Renderer/Mesh.h"
#include "Engine/Scene/BoundingBox.h"
#include "Engine/Scene/Transform.h"

#include <memory>
#include <string>


/**
 * @brief Объект, находящийся внутри Scene.
 *
 * SceneObject объединяет данные одного объекта сцены:
 *
 * - имя;
 * - Transform;
 * - Mesh;
 * - локальный BoundingBox.
 *
 * Сам объект не занимается рендерингом.
 * Его данные используются SceneViewport и Renderer.
 */
class SceneObject {
public:

    /**
     * @brief Создаёт пустой объект сцены.
     */
    SceneObject() = default;


    /**
     * @brief Создаёт объект с указанным именем.
     *
     * @param name Имя объекта.
     */
    explicit SceneObject(
        std::string name
    );


    /**
     * @brief Создаёт объект с именем и Mesh.
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
     */
    const std::string&
    GetName() const;


    /**
     * @brief Изменяет имя объекта.
     *
     * @param name Новое имя.
     */
    void SetName(
        std::string name
    );


    /**
     * @brief Возвращает изменяемый Transform.
     *
     * Используется Editor для изменения
     * Position / Rotation / Scale.
     */
    Transform&
    GetTransform();


    /**
     * @brief Возвращает Transform только для чтения.
     */
    const Transform&
    GetTransform() const;


    /**
     * @brief Устанавливает Mesh объекта.
     */
    void SetMesh(
        std::shared_ptr<Mesh> mesh
    );


    /**
     * @brief Возвращает Mesh объекта.
     */
    std::shared_ptr<Mesh>
    GetMesh();


    /**
     * @brief Возвращает Mesh только для чтения.
     */
    std::shared_ptr<const Mesh>
    GetMesh() const;


    /**
     * @brief Проверяет наличие Mesh.
     */
    bool HasMesh() const;


    /**
     * @brief Устанавливает локальный BoundingBox объекта.
     *
     * BoundingBox хранится до применения Transform,
     * то есть в Local Space Mesh.
     */
    void SetBoundingBox(
        const BoundingBox& bounding_box
    );


    /**
     * @brief Возвращает локальный BoundingBox.
     */
    const BoundingBox&
    GetBoundingBox() const;


private:

    /**
     * @brief Имя объекта в Hierarchy.
     */
    std::string name_{
        "SceneObject"
    };


    /**
     * @brief Пространственное преобразование объекта.
     */
    Transform transform_{};


    /**
     * @brief Геометрия объекта.
     */
    std::shared_ptr<Mesh>
        mesh_;


    /**
     * @brief AABB геометрии в Local Space.
     */
    BoundingBox bounding_box_{};
};