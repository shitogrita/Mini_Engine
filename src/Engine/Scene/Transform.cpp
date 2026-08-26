#include "Transform.h"

#include "../Math/affine_transformation.h"


Matrix4 Transform::GetModelMatrix() const
{
	return AffineTransformation::ComposeModelMatrix(
		position,
		rotation,
		scale
	);
}