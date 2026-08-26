#pragma once

#include "Engine/Scene/SceneObject.h"

#include <memory>
#include <vector>


class Scene {
public:
	void AddObject(
		std::shared_ptr<SceneObject> object
	);

	void Clear();

	const std::vector<
		std::shared_ptr<SceneObject>
	>& GetObjects() const;

private:
	std::vector<
		std::shared_ptr<SceneObject>
	> objects_;
};