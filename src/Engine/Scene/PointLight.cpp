#include "Engine/Scene/PointLight.h"

#include <algorithm>

/**
 * @brief Возвращает позицию PointLight.
 */
const Vec3& PointLight::GetPosition() const {
	return position_;
}

/**
 * @brief Изменяет позицию PointLight.
 *
 * @param position Новая позиция источника.
 */
void PointLight::SetPosition(const Vec3& position) {
	position_ = position;
}

/**
 * @brief Возвращает цвет PointLight.
 */
const Vec3& PointLight::GetColor() const {
	return color_;
}

/**
 * @brief Изменяет цвет PointLight.
 *
 * @param color Новый RGB-цвет.
 */
void PointLight::SetColor(const Vec3& color) {
	color_ = color;
}

/**
 * @brief Возвращает интенсивность PointLight.
 */
float PointLight::GetIntensity() const {
	return intensity_;
}

/**
 * @brief Изменяет интенсивность PointLight.
 *
 * Интенсивность не может быть отрицательной.
 *
 * @param intensity Новая интенсивность.
 */
void PointLight::SetIntensity(float intensity) {
	intensity_ = std::max(0.0f, intensity);
}

/**
 * @brief Возвращает состояние PointLight.
 */
bool PointLight::IsEnabled() const {
	return enabled_;
}

/**
 * @brief Включает или выключает PointLight.
 *
 * @param enabled Новое состояние.
 */
void PointLight::SetEnabled(bool enabled) {
	enabled_ = enabled;
}