#include "SceneObject.h"

#include <utility>

SceneObject::SceneObject(std::string name): name_(std::move(name)) {
}

SceneObject::SceneObject(std::string name, std::shared_ptr<Mesh> mesh): name_(std::move(name)), mesh_(std::move(mesh)) {
}

const std::string& SceneObject::GetName() const {
	return name_;
}

void SceneObject::SetName(std::string name) {
	name_ =
		std::move(name);
}

const Transform &SceneObject::GetTransform() const {
	return transform_;
}

void SceneObject::SetMesh(std::shared_ptr<Mesh> mesh) {
	/*
	 * SceneObject получает shared_ptr на Mesh.
	 *
	 * Сам Mesh будет существовать до тех пор,
	 * пока на него существует хотя бы один shared_ptr.
	 */
	mesh_ =
		std::move(mesh);
}

std::shared_ptr<Mesh> SceneObject::GetMesh() {
	return mesh_;
}

std::shared_ptr<const Mesh> SceneObject::GetMesh() const {
	return mesh_;
}

bool SceneObject::HasMesh() const {
	/*
	 * shared_ptr можно преобразовать в bool:
	 *
	 * true  — указатель содержит объект;
	 * false — указатель пустой.
	 */
	return static_cast<bool>(
		mesh_
	);
}

void SceneObject::SetBoundingBox( const BoundingBox& bounding_box) {
	/*
	 * Сохраняем границы исходной геометрии объекта.
	 *
	 * Transform здесь не применяется.
	 * Поэтому BoundingBox пока остаётся
	 * в локальной системе координат Mesh.
	 */
	bounding_box_ =
		bounding_box;
}

const BoundingBox& SceneObject::GetBoundingBox() const {
	/*
	 * Возвращаем константную ссылку,
	 * потому что внешнему коду обычно нужно
	 * только проверить границы объекта,
	 * а не менять их напрямую.
	 */
	return bounding_box_;
}