#include "Scene.h"

#include <algorithm>
#include <utility>

void Scene::AddObject(std::shared_ptr<SceneObject> object) {
	objects_.push_back(
		std::move(object)
	);
}

bool Scene::RemoveObject(const std::shared_ptr<SceneObject>& object) {
	if (!object) {
		return false;
	}

	const auto iterator = std::find(
		objects_.begin(),
		objects_.end(),
		object
	);

	if (iterator == objects_.end()) {
		return false;
	}

	objects_.erase(iterator);

	return true;
}

void Scene::Clear() {
	objects_.clear();
}

const std::vector<std::shared_ptr<SceneObject>>& Scene::GetObjects() const {
	return objects_;
}