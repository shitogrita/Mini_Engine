#pragma once

#include "../Renderer/Mesh.h"
#include "../Scene/Transform.h"

#include <memory>
#include <string>


class SceneObject {
public:
	SceneObject(
		std::string name,
		std::shared_ptr<Mesh> mesh
	);

	const std::string& GetName() const;

	void SetName(
		std::string name
	);

	Transform& GetTransform();
	const Transform& GetTransform() const;

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<const Mesh> GetMesh() const;

private:
	std::string name_;

	Transform transform_;

	std::shared_ptr<Mesh> mesh_;
};