#pragma once

#include "Engine/Renderer/Material.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Scene/BoundingBox.h"
#include "Engine/Scene/PointLight.h"
#include "Engine/Scene/Transform.h"
#include "Engine/Scene/MotionState.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Универсальный объект сцены.
 *
 * SceneObject хранит:
 * - имя;
 * - Transform;
 * - Mesh или набор RenderPart;
 * - Material;
 * - BoundingBox;
 * - тип объекта;
 * - путь к исходной модели;
 * - опциональный PointLight.
 *
 * PointLight хранится непосредственно внутри SceneObject.
 * Благодаря этому источник света является полноценным
 * объектом Scene и не требует отдельного глобального
 * состояния внутри SceneViewport.
 */
class SceneObject {
public:
    /**
     * @brief Тип объекта сцены.
     *
     * Тип используется Editor для определения
     * специфического поведения объекта.
     */
    enum class Type {
        Empty,
        Cube,
        Plane,
        Sphere,
        ImportedModel,
        PointLight
    };

    /**
     * @brief Отдельная часть multipart-модели.
     *
     * Каждая часть может иметь собственный Mesh
     * и собственный Material.
     */
    struct SceneRenderPart {
        std::string name;
        std::shared_ptr<Mesh> mesh;
        Material material;
    };

    SceneObject() = default;

    /**
     * @brief Создаёт объект сцены без Mesh.
     *
     * @param name Имя объекта.
     */
    explicit SceneObject(std::string name);

    /**
     * @brief Создаёт объект сцены с Mesh.
     *
     * @param name Имя объекта.
     * @param mesh Геометрия объекта.
     */
    SceneObject(std::string name, std::shared_ptr<Mesh> mesh);

    /**
     * @brief Возвращает имя объекта.
     */
    const std::string& GetName() const;

    /**
     * @brief Изменяет имя объекта.
     */
    void SetName(std::string name);

    /**
     * @brief Возвращает изменяемый Transform.
     */
    Transform& GetTransform();

    /**
     * @brief Возвращает Transform только для чтения.
     */
    const Transform& GetTransform() const;

    /**
     * @brief Устанавливает основной Mesh.
     */
    void SetMesh(std::shared_ptr<Mesh> mesh);

    /**
     * @brief Возвращает основной Mesh.
     */
    std::shared_ptr<Mesh> GetMesh();

    /**
     * @brief Возвращает основной Mesh только для чтения.
     */
    std::shared_ptr<const Mesh> GetMesh() const;

    /**
     * @brief Проверяет наличие геометрии.
     *
     * Multipart RenderPart также считаются геометрией объекта.
     */
    bool HasMesh() const;

    /**
     * @brief Устанавливает локальный BoundingBox.
     */
    void SetBoundingBox(const BoundingBox& bounding_box);

    /**
     * @brief Возвращает BoundingBox.
     */
    const BoundingBox& GetBoundingBox() const;

    /**
     * @brief Возвращает основной Material.
     */
    Material& GetMaterial();

    /**
     * @brief Возвращает основной Material только для чтения.
     */
    const Material& GetMaterial() const;

    /**
     * @brief Добавляет RenderPart multipart-модели.
     *
     * @param name Имя части.
     * @param mesh Геометрия части.
     * @param material Материал части.
     */
    void AddRenderPart(std::string name, std::shared_ptr<Mesh> mesh, Material material);

    /**
     * @brief Возвращает RenderPart.
     */
    std::vector<SceneRenderPart>& GetRenderParts();

    /**
     * @brief Возвращает RenderPart только для чтения.
     */
    const std::vector<SceneRenderPart>& GetRenderParts() const;

    /**
     * @brief Проверяет наличие multipart-геометрии.
     */
    bool HasRenderParts() const;

    /**
     * @brief Устанавливает тип объекта.
     */
    void SetType(Type type);

    /**
     * @brief Возвращает тип объекта.
     */
    Type GetType() const;

    /**
     * @brief Сохраняет путь к исходному ресурсу.
     */
    void SetSourcePath(std::filesystem::path path);

    /**
     * @brief Возвращает путь к исходному ресурсу.
     */
    const std::filesystem::path& GetSourcePath() const;

    /**
     * @brief Добавляет или заменяет компонент PointLight.
     *
     * После этого объект становится полноценным
     * носителем параметров источника света.
     *
     * @param light Параметры источника света.
     */
    void SetPointLight(const PointLight& light);

    /**
     * @brief Возвращает изменяемый PointLight.
     *
     * @return Указатель на компонент или nullptr.
     */
    PointLight* GetPointLight();

    /**
     * @brief Возвращает PointLight только для чтения.
     *
     * @return Указатель на компонент или nullptr.
     */
    const PointLight* GetPointLight() const;

    /**
     * @brief Проверяет наличие компонента PointLight.
     */

    bool HasPointLight() const;
    /**
     * @brief Возвращает изменяемое состояние движения объекта.
     */
    MotionState& GetMotionState();

    /**
     * @brief Возвращает состояние движения только для чтения.
     */
    const MotionState& GetMotionState() const;


private:
    std::string name_{"SceneObject"};

    Transform transform_{};

    MotionState motion_state_{};

    std::shared_ptr<Mesh> mesh_;

    BoundingBox bounding_box_{};

    Material material_{};

    std::vector<SceneRenderPart> render_parts_;

    Type type_ = Type::Empty;

    std::filesystem::path source_path_;

    /**
     * @brief Компонент источника света.
     *
     * std::nullopt означает, что объект
     * не является источником света.
     */
    std::optional<PointLight> point_light_;
};