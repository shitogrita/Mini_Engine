#include "SceneObject.h"

#include <utility>

SceneObject::SceneObject(
	std::string name,
	std::shared_ptr<Mesh> mesh
)
	: name_(std::move(name)),
	  mesh_(std::move(mesh))
{
}

const std::string& SceneObject::GetName() const
{
	return name_;
}


void SceneObject::SetName(
	std::string name
)
{
	name_ = std::move(name);
}


Transform& SceneObject::GetTransform()
{
	return transform_;
}


const Transform& SceneObject::GetTransform() const
{
	return transform_;
}


std::shared_ptr<Mesh> SceneObject::GetMesh()
{
	return mesh_;
}


std::shared_ptr<const Mesh> SceneObject::GetMesh() const
{
	return mesh_;
}