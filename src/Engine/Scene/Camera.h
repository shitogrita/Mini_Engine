#pragma once

#include "../Math/matrix_types.h"

class Camera {
public:
	void MoveForward(float distance);
	void MoveBackward(float distance);
	void MoveLeft(float distance);
	void MoveRight(float distance);
	void MoveUp(float distance);
	void MoveDown(float distance);

	Matrix4 GetViewMatrix() const;

private:
	Vec3 position_{
		0.0f,
		0.0f,
		3.0f
	};
};