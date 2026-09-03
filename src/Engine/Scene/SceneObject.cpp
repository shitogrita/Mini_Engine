#include "Engine/Scene/SceneObject.h"

#include <utility>


SceneObject::SceneObject(
	std::string name
)
	: name_(
		std::move(name)
	)
{
}


SceneObject::SceneObject(
	std::string name,
	std::shared_ptr<Mesh> mesh
)
	: name_(
		std::move(name)
	),
	  mesh_(
		  std::move(mesh)
	  )
{
}


const std::string&
SceneObject::GetName() const
{
	return name_;
}


void SceneObject::SetName(
	std::string name
)
{
	name_ =
		std::move(name);
}


Transform&
SceneObject::GetTransform()
{
	return transform_;
}


const Transform&
SceneObject::GetTransform() const
{
	return transform_;
}


void SceneObject::SetMesh(
	std::shared_ptr<Mesh> mesh
)
{
	mesh_ =
		std::move(mesh);
}


std::shared_ptr<Mesh>
SceneObject::GetMesh()
{
	return mesh_;
}


std::shared_ptr<const Mesh>
SceneObject::GetMesh() const
{
	return mesh_;
}


bool SceneObject::HasMesh() const
{
	return mesh_ != nullptr;
}


void SceneObject::SetBoundingBox(
	const BoundingBox& bounding_box
)
{
	bounding_box_ =
		bounding_box;
}


const BoundingBox&
SceneObject::GetBoundingBox() const
{
	return bounding_box_;
}