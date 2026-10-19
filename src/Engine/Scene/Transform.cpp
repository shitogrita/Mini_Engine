#include "Transform.h"

#include "../Math/affine_transformation.h"

/**
 * @brief Формирует Model Matrix объекта.
 *
 * Матрица строится на основе текущих компонентов Transform:
 *
 * - position;
 * - rotation;
 * - scale.
 *
 * Для построения используется
 * AffineTransformation::ComposeModelMatrix().
 *
 * В текущей реализации порядок преобразований
 * определяется внутри ComposeModelMatrix().
 *
 * @return Model Matrix текущего Transform.
 */
Matrix4 Transform::GetModelMatrix() const {
	return AffineTransformation::ComposeModelMatrix(
		position,
		rotation,
		scale
	);
}