#include "Camera.h"

#include "../Math/affine_transformation.h"


void Camera::MoveForward(float distance)
{
	position_.z -= distance;
}


void Camera::MoveBackward(float distance)
{
	position_.z += distance;
}


void Camera::MoveLeft(float distance)
{
	position_.x -= distance;
}


void Camera::MoveRight(float distance)
{
	position_.x += distance;
}


void Camera::MoveUp(float distance)
{
	position_.y += distance;
}


void Camera::MoveDown(float distance)
{
	position_.y -= distance;
}


Matrix4 Camera::GetViewMatrix() const
{
	return AffineTransformation::Translation4(
		-position_.x,
		-position_.y,
		-position_.z
	);
}