#include "Engine/Scene/SceneObject.h"

#include <utility>

/**
 * @brief Создаёт SceneObject без геометрии.
 *
 * @param name Имя объекта.
 */
SceneObject::SceneObject(std::string name) : name_(std::move(name)) {
}

/**
 * @brief Создаёт SceneObject с основным Mesh.
 *
 * @param name Имя объекта.
 * @param mesh Геометрия объекта.
 */
SceneObject::SceneObject(std::string name, std::shared_ptr<Mesh> mesh)
    : name_(std::move(name)),
      mesh_(std::move(mesh)) {
}

const std::string& SceneObject::GetName() const {
    return name_;
}

void SceneObject::SetName(std::string name) {
    name_ = std::move(name);
}

Transform& SceneObject::GetTransform() {
    return transform_;
}

const Transform& SceneObject::GetTransform() const {
    return transform_;
}

void SceneObject::SetMesh(std::shared_ptr<Mesh> mesh) {
    mesh_ = std::move(mesh);
}

std::shared_ptr<Mesh> SceneObject::GetMesh() {
    return mesh_;
}

std::shared_ptr<const Mesh> SceneObject::GetMesh() const {
    return mesh_;
}

bool SceneObject::HasMesh() const {
    return mesh_ != nullptr || !render_parts_.empty();
}

void SceneObject::SetBoundingBox(const BoundingBox& bounding_box) {
    bounding_box_ = bounding_box;
}

const BoundingBox& SceneObject::GetBoundingBox() const {
    return bounding_box_;
}

Material& SceneObject::GetMaterial() {
    return material_;
}

const Material& SceneObject::GetMaterial() const {
    return material_;
}

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

std::vector<SceneObject::SceneRenderPart>& SceneObject::GetRenderParts() {
    return render_parts_;
}

const std::vector<SceneObject::SceneRenderPart>& SceneObject::GetRenderParts() const {
    return render_parts_;
}

bool SceneObject::HasRenderParts() const {
    return !render_parts_.empty();
}

void SceneObject::SetType(Type type) {
    type_ = type;
}

SceneObject::Type SceneObject::GetType() const {
    return type_;
}

void SceneObject::SetSourcePath(std::filesystem::path path) {
    source_path_ = std::move(path);
}

const std::filesystem::path& SceneObject::GetSourcePath() const {
    return source_path_;
}

/**
 * @brief Добавляет или заменяет PointLight.
 *
 * @param light Параметры источника света.
 */
void SceneObject::SetPointLight(const PointLight& light) {
    point_light_ = light;
}

/**
 * @brief Возвращает изменяемый PointLight.
 *
 * @return PointLight или nullptr.
 */
PointLight* SceneObject::GetPointLight() {
    if (!point_light_.has_value()) {
        return nullptr;
    }

    return &point_light_.value();
}

/**
 * @brief Возвращает PointLight только для чтения.
 *
 * @return PointLight или nullptr.
 */
const PointLight* SceneObject::GetPointLight() const {
    if (!point_light_.has_value()) {
        return nullptr;
    }

    return &point_light_.value();
}

/**
 * @brief Проверяет наличие PointLight.
 */
bool SceneObject::HasPointLight() const {
    return point_light_.has_value();
}