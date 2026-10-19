#include "Engine/Scene/SceneObject.h"

#include <utility>

/**
 * @brief Создаёт SceneObject без основной геометрии.
 *
 * Объект получает указанное имя,
 * а остальные данные остаются
 * в состояниях по умолчанию,
 * определённых в SceneObject.
 *
 * @param name Имя объекта.
 */
SceneObject::SceneObject(std::string name) : name_(std::move(name)) {
}

/**
 * @brief Создаёт SceneObject с основным Mesh.
 *
 * Имя и shared_ptr на геометрию
 * перемещаются во внутренние поля объекта.
 *
 * @param name Имя объекта.
 * @param mesh Основная геометрия объекта.
 */
SceneObject::SceneObject(std::string name, std::shared_ptr<Mesh> mesh)
    : name_(std::move(name)),
      mesh_(std::move(mesh)) {
}

/**
 * @brief Возвращает имя SceneObject.
 *
 * @return Константная ссылка на имя объекта.
 */
const std::string& SceneObject::GetName() const {
    return name_;
}

/**
 * @brief Изменяет имя SceneObject.
 *
 * Переданная строка перемещается
 * во внутреннее поле name_.
 *
 * @param name Новое имя объекта.
 */
void SceneObject::SetName(std::string name) {
    name_ = std::move(name);
}

/**
 * @brief Возвращает изменяемый Transform объекта.
 *
 * Через возвращаемую ссылку можно изменять
 * позицию, вращение и масштаб SceneObject.
 *
 * @return Ссылка на transform_.
 */
Transform& SceneObject::GetTransform() {
    return transform_;
}

/**
 * @brief Возвращает Transform объекта только для чтения.
 *
 * @return Константная ссылка на transform_.
 */
const Transform& SceneObject::GetTransform() const {
    return transform_;
}

/**
 * @brief Возвращает изменяемое состояние движения объекта.
 *
 * MotionState содержит параметры,
 * используемые системой обновления движения SceneObject.
 *
 * @return Ссылка на motion_state_.
 */
MotionState& SceneObject::GetMotionState() {
    return motion_state_;
}

/**
 * @brief Возвращает состояние движения объекта только для чтения.
 *
 * @return Константная ссылка на motion_state_.
 */
const MotionState& SceneObject::GetMotionState() const {
    return motion_state_;
}

/**
 * @brief Устанавливает основной Mesh SceneObject.
 *
 * Переданный std::shared_ptr перемещается
 * во внутреннее поле mesh_.
 *
 * @param mesh Новая основная геометрия объекта.
 */
void SceneObject::SetMesh(std::shared_ptr<Mesh> mesh) {
    mesh_ = std::move(mesh);
}

/**
 * @brief Возвращает основной Mesh SceneObject.
 *
 * Возвращается копия std::shared_ptr,
 * поэтому владение Mesh разделяется
 * с вызывающим кодом.
 *
 * @return shared_ptr на основной Mesh.
 */
std::shared_ptr<Mesh> SceneObject::GetMesh() {
    return mesh_;
}

/**
 * @brief Возвращает основной Mesh только для чтения.
 *
 * Возвращается std::shared_ptr<const Mesh>,
 * запрещающий изменение Mesh через полученный указатель.
 *
 * @return shared_ptr на константный Mesh.
 */
std::shared_ptr<const Mesh> SceneObject::GetMesh() const {
    return mesh_;
}

/**
 * @brief Проверяет наличие геометрии у SceneObject.
 *
 * Метод возвращает true,
 * если выполнено хотя бы одно условие:
 *
 * - установлен основной mesh_;
 * - render_parts_ содержит хотя бы одну часть.
 *
 * Таким образом, объект считается имеющим геометрию
 * как при обычном одном Mesh,
 * так и при multipart-модели.
 *
 * @return true, если объект содержит геометрию,
 * иначе false.
 */
bool SceneObject::HasMesh() const {
    return mesh_ != nullptr || !render_parts_.empty();
}

/**
 * @brief Устанавливает локальный BoundingBox объекта.
 *
 * @param bounding_box Новый ограничивающий параллелепипед.
 */
void SceneObject::SetBoundingBox(const BoundingBox& bounding_box) {
    bounding_box_ = bounding_box;
}

/**
 * @brief Возвращает BoundingBox SceneObject.
 *
 * @return Константная ссылка на bounding_box_.
 */
const BoundingBox& SceneObject::GetBoundingBox() const {
    return bounding_box_;
}

/**
 * @brief Возвращает изменяемый основной Material объекта.
 *
 * Этот Material относится к основной геометрии mesh_.
 *
 * Render parts содержат собственные Material
 * внутри SceneRenderPart.
 *
 * @return Ссылка на material_.
 */
Material& SceneObject::GetMaterial() {
    return material_;
}

/**
 * @brief Возвращает основной Material объекта только для чтения.
 *
 * @return Константная ссылка на material_.
 */
const Material& SceneObject::GetMaterial() const {
    return material_;
}

/**
 * @brief Добавляет отдельную часть модели для рендеринга.
 *
 * SceneRenderPart объединяет:
 *
 * - имя части;
 * - Mesh;
 * - Material.
 *
 * Такая структура используется для моделей,
 * состоящих из нескольких геометрических частей
 * с разными материалами.
 *
 * Если mesh равен nullptr,
 * новая render part не добавляется.
 *
 * @param name Имя render part.
 * @param mesh Геометрия части модели.
 * @param material Материал данной части.
 */
void SceneObject::AddRenderPart(std::string name, std::shared_ptr<Mesh> mesh, Material material) {
    if (!mesh) {
        return;
    }

    render_parts_.push_back(
        SceneRenderPart{
            std::move(name),
            std::move(mesh),
            std::move(material)
        }
    );
}

/**
 * @brief Возвращает изменяемый список render parts.
 *
 * @return Ссылка на render_parts_.
 */
std::vector<SceneObject::SceneRenderPart>& SceneObject::GetRenderParts() {
    return render_parts_;
}

/**
 * @brief Возвращает список render parts только для чтения.
 *
 * @return Константная ссылка на render_parts_.
 */
const std::vector<SceneObject::SceneRenderPart>& SceneObject::GetRenderParts() const {
    return render_parts_;
}

/**
 * @brief Проверяет наличие отдельных render parts.
 *
 * @return true, если render_parts_ не пуст,
 * иначе false.
 */
bool SceneObject::HasRenderParts() const {
    return !render_parts_.empty();
}

/**
 * @brief Устанавливает тип SceneObject.
 *
 * @param type Новый тип объекта.
 */
void SceneObject::SetType(Type type) {
    type_ = type;
}

/**
 * @brief Возвращает тип SceneObject.
 *
 * @return Текущее значение type_.
 */
SceneObject::Type SceneObject::GetType() const {
    return type_;
}

/**
 * @brief Сохраняет путь к исходному ресурсу объекта.
 *
 * Переданный std::filesystem::path
 * перемещается в source_path_.
 *
 * @param path Путь к исходному ресурсу.
 */
void SceneObject::SetSourcePath(std::filesystem::path path) {
    source_path_ = std::move(path);
}

/**
 * @brief Возвращает путь к исходному ресурсу SceneObject.
 *
 * @return Константная ссылка на source_path_.
 */
const std::filesystem::path& SceneObject::GetSourcePath() const {
    return source_path_;
}

/**
 * @brief Добавляет или заменяет PointLight SceneObject.
 *
 * PointLight хранится внутри объекта
 * как optional-компонент.
 *
 * Если PointLight уже существует,
 * его значение заменяется переданным light.
 *
 * @param light Параметры источника света.
 */
void SceneObject::SetPointLight(const PointLight& light) {
    point_light_ = light;
}

/**
 * @brief Возвращает изменяемый PointLight SceneObject.
 *
 * Если optional-компонент PointLight отсутствует,
 * возвращается nullptr.
 *
 * @return Указатель на PointLight
 * или nullptr при отсутствии компонента.
 */
PointLight* SceneObject::GetPointLight() {
    if (!point_light_.has_value()) {
        return nullptr;
    }

    return &point_light_.value();
}

/**
 * @brief Возвращает PointLight SceneObject только для чтения.
 *
 * Если optional-компонент PointLight отсутствует,
 * возвращается nullptr.
 *
 * @return Константный указатель на PointLight
 * или nullptr при отсутствии компонента.
 */
const PointLight* SceneObject::GetPointLight() const {
    if (!point_light_.has_value()) {
        return nullptr;
    }

    return &point_light_.value();
}

/**
 * @brief Проверяет наличие PointLight у SceneObject.
 *
 * @return true, если point_light_ содержит значение,
 * иначе false.
 */
bool SceneObject::HasPointLight() const {
    return point_light_.has_value();
}