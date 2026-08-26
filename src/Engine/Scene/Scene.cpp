#include "Scene.h"

#include <utility>


void Scene::AddObject(
	std::shared_ptr<SceneObject> object
)
{
	objects_.push_back(
		std::move(object)
	);
}


void Scene::Clear()
{
	objects_.clear();
}


const std::vector<
	std::shared_ptr<SceneObject>
>& Scene::GetObjects() const
{
	return objects_;
}