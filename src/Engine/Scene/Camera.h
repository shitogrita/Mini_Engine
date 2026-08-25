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

	void Rotate(
		float yaw_delta_degrees,
		float pitch_delta_degrees
	);

	Matrix4 GetViewMatrix() const;

	const Vec3& GetPosition() const;

	float GetYaw() const;
	float GetPitch() const;


private:
	Vec3 GetForwardDirection() const;
	Vec3 GetRightDirection() const;

private:

	float yaw_degrees_ = -90.0f;;  // поворот влево/вправо
	float pitch_degrees_ = 0.0f;; // поворот вверх/вниз
	Vec3 position_{
		0.0f,
		0.0f,
		3.0f
	};
};