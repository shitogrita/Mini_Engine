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
 * SceneObject является основной сущностью,
 * размещаемой внутри Scene.
 *
 * Объект может хранить:
 *
 * - имя;
 * - Transform;
 * - состояние движения MotionState;
 * - основной Mesh;
 * - основной Material;
 * - BoundingBox;
 * - набор SceneRenderPart;
 * - тип объекта;
 * - путь к исходному ресурсу;
 * - опциональный компонент PointLight.
 *
 * Простые примитивы могут использовать основной mesh_
 * и material_, а импортированные multipart-модели
 * могут состоять из нескольких SceneRenderPart,
 * каждый из которых содержит собственные Mesh и Material.
 *
 * PointLight хранится непосредственно внутри SceneObject
 * через std::optional<PointLight>.
 *
 * Благодаря этому источник света может быть представлен
 * обычным объектом Scene вместе с другими сущностями.
 */
class SceneObject {
public:
    /**
     * @brief Определяет тип объекта сцены.
     *
     * Тип может использоваться Editor
     * и другими системами для определения
     * назначения SceneObject.
     */
    enum class Type {
        /**
         * @brief Объект без специализированного типа.
         */
        Empty,

        /**
         * @brief Кубический примитив.
         */
        Cube,

        /**
         * @brief Плоскость.
         */
        Plane,

        /**
         * @brief Сферический примитив.
         */
        Sphere,

        /**
         * @brief Модель, импортированная из внешнего файла.
         */
        ImportedModel,

        /**
         * @brief Объект, представляющий точечный источник света.
         */
        PointLight
    };

    /**
     * @brief Отдельная часть multipart-модели.
     *
     * SceneRenderPart объединяет:
     *
     * - имя части;
     * - Mesh;
     * - Material.
     *
     * Это позволяет одному SceneObject
     * содержать несколько частей геометрии
     * с разными материалами.
     */
    struct SceneRenderPart {
        /**
         * @brief Имя части модели.
         */
        std::string name;

        /**
         * @brief Геометрия данной части модели.
         */
        std::shared_ptr<Mesh> mesh;

        /**
         * @brief Материал данной части модели.
         */
        Material material;
    };

    /**
     * @brief Создаёт SceneObject
     * со значениями полей по умолчанию.
     */
    SceneObject() = default;

    /**
     * @brief Создаёт SceneObject без основного Mesh.
     *
     * @param name Имя объекта.
     */
    explicit SceneObject(std::string name);

    /**
     * @brief Создаёт SceneObject с основным Mesh.
     *
     * @param name Имя объекта.
     * @param mesh Геометрия объекта.
     */
    SceneObject(std::string name, std::shared_ptr<Mesh> mesh);

    /**
     * @brief Возвращает имя объекта.
     *
     * @return Константная ссылка на name_.
     */
    const std::string& GetName() const;

    /**
     * @brief Изменяет имя объекта.
     *
     * @param name Новое имя SceneObject.
     */
    void SetName(std::string name);

    /**
     * @brief Возвращает изменяемый Transform объекта.
     *
     * @return Ссылка на transform_.
     */
    Transform& GetTransform();

    /**
     * @brief Возвращает Transform объекта только для чтения.
     *
     * @return Константная ссылка на transform_.
     */
    const Transform& GetTransform() const;

    /**
     * @brief Устанавливает основной Mesh объекта.
     *
     * @param mesh Новая основная геометрия SceneObject.
     */
    void SetMesh(std::shared_ptr<Mesh> mesh);

    /**
     * @brief Возвращает основной Mesh объекта.
     *
     * Возвращается копия std::shared_ptr,
     * поэтому владение Mesh разделяется
     * с вызывающим кодом.
     *
     * @return shared_ptr на основной Mesh.
     */
    std::shared_ptr<Mesh> GetMesh();

    /**
     * @brief Возвращает основной Mesh только для чтения.
     *
     * @return shared_ptr на const Mesh.
     */
    std::shared_ptr<const Mesh> GetMesh() const;

    /**
     * @brief Проверяет наличие геометрии у SceneObject.
     *
     * Метод возвращает true,
     * если установлен основной mesh_
     * или render_parts_ содержит хотя бы один элемент.
     *
     * @return true, если объект содержит геометрию,
     * иначе false.
     */
    bool HasMesh() const;

    /**
     * @brief Устанавливает локальный BoundingBox объекта.
     *
     * @param bounding_box Новый BoundingBox.
     */
    void SetBoundingBox(const BoundingBox& bounding_box);

    /**
     * @brief Возвращает локальный BoundingBox объекта.
     *
     * @return Константная ссылка на bounding_box_.
     */
    const BoundingBox& GetBoundingBox() const;

    /**
     * @brief Возвращает изменяемый основной Material объекта.
     *
     * Для multipart-моделей отдельные части
     * могут иметь собственные Material
     * внутри SceneRenderPart.
     *
     * @return Ссылка на material_.
     */
    Material& GetMaterial();

    /**
     * @brief Возвращает основной Material только для чтения.
     *
     * @return Константная ссылка на material_.
     */
    const Material& GetMaterial() const;

    /**
     * @brief Добавляет RenderPart multipart-модели.
     *
     * Если mesh равен nullptr,
     * новая часть не добавляется.
     *
     * @param name Имя части.
     * @param mesh Геометрия части.
     * @param material Материал части.
     */
    void AddRenderPart(std::string name, std::shared_ptr<Mesh> mesh, Material material);

    /**
     * @brief Возвращает изменяемый список RenderPart.
     *
     * @return Ссылка на render_parts_.
     */
    std::vector<SceneRenderPart>& GetRenderParts();

    /**
     * @brief Возвращает список RenderPart только для чтения.
     *
     * @return Константная ссылка на render_parts_.
     */
    const std::vector<SceneRenderPart>& GetRenderParts() const;

    /**
     * @brief Проверяет наличие multipart-геометрии.
     *
     * @return true, если render_parts_ не пуст,
     * иначе false.
     */
    bool HasRenderParts() const;

    /**
     * @brief Устанавливает тип SceneObject.
     *
     * @param type Новый тип объекта.
     */
    void SetType(Type type);

    /**
     * @brief Возвращает тип SceneObject.
     *
     * @return Текущее значение type_.
     */
    Type GetType() const;

    /**
     * @brief Сохраняет путь к исходному ресурсу объекта.
     *
     * @param path Путь к исходному ресурсу.
     */
    void SetSourcePath(std::filesystem::path path);

    /**
     * @brief Возвращает путь к исходному ресурсу объекта.
     *
     * @return Константная ссылка на source_path_.
     */
    const std::filesystem::path& GetSourcePath() const;

    /**
     * @brief Добавляет или заменяет компонент PointLight.
     *
     * PointLight хранится внутри SceneObject
     * через std::optional.
     *
     * Если компонент уже существует,
     * его значение заменяется.
     *
     * @param light Параметры источника света.
     */
    void SetPointLight(const PointLight& light);

    /**
     * @brief Возвращает изменяемый компонент PointLight.
     *
     * Если point_light_ не содержит значения,
     * возвращается nullptr.
     *
     * @return Указатель на PointLight
     * или nullptr при отсутствии компонента.
     */
    PointLight* GetPointLight();

    /**
     * @brief Возвращает компонент PointLight только для чтения.
     *
     * Если point_light_ не содержит значения,
     * возвращается nullptr.
     *
     * @return Константный указатель на PointLight
     * или nullptr при отсутствии компонента.
     */
    const PointLight* GetPointLight() const;

    /**
     * @brief Проверяет наличие компонента PointLight.
     *
     * @return true, если point_light_ содержит значение,
     * иначе false.
     */
    bool HasPointLight() const;

    /**
     * @brief Возвращает изменяемое состояние движения объекта.
     *
     * @return Ссылка на motion_state_.
     */
    MotionState& GetMotionState();

    /**
     * @brief Возвращает состояние движения объекта только для чтения.
     *
     * @return Константная ссылка на motion_state_.
     */
    const MotionState& GetMotionState() const;

private:
    /**
     * @brief Имя объекта сцены.
     *
     * По умолчанию используется "SceneObject".
     */
    std::string name_{"SceneObject"};

    /**
     * @brief Пространственное преобразование объекта.
     */
    Transform transform_{};

    /**
     * @brief Параметры движения объекта.
     */
    MotionState motion_state_{};

    /**
     * @brief Основная геометрия SceneObject.
     *
     * Может быть пустой,
     * если объект не имеет Mesh
     * или использует только render_parts_.
     */
    std::shared_ptr<Mesh> mesh_;

    /**
     * @brief Локальный BoundingBox объекта.
     */
    BoundingBox bounding_box_{};

    /**
     * @brief Основной Material объекта.
     */
    Material material_{};

    /**
     * @brief Части multipart-модели.
     *
     * Каждая SceneRenderPart содержит
     * собственные Mesh и Material.
     */
    std::vector<SceneRenderPart> render_parts_;

    /**
     * @brief Тип объекта сцены.
     *
     * По умолчанию объект имеет Type::Empty.
     */
    Type type_ = Type::Empty;

    /**
     * @brief Путь к исходному ресурсу объекта.
     *
     * Используется для хранения информации
     * об источнике импортированной модели.
     */
    std::filesystem::path source_path_;

    /**
     * @brief Опциональный компонент PointLight.
     *
     * std::nullopt означает,
     * что компонент PointLight
     * у данного SceneObject отсутствует.
     */
    std::optional<PointLight> point_light_;
};