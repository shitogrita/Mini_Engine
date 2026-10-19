#include "Engine/Scene/PointLight.h"

#include <algorithm>

/**
 * @brief Возвращает позицию PointLight.
 *
 * Позиция хранится в мировом пространстве
 * и используется при расчёте освещения сцены.
 *
 * @return Константная ссылка на текущую позицию источника света.
 */
const Vec3& PointLight::GetPosition() const {
    return position_;
}

/**
 * @brief Изменяет позицию PointLight.
 *
 * @param position Новая позиция источника света
 * в мировом пространстве.
 */
void PointLight::SetPosition(const Vec3& position) {
    position_ = position;
}

/**
 * @brief Возвращает цвет PointLight.
 *
 * Цвет представлен тремя компонентами Vec3,
 * соответствующими RGB.
 *
 * @return Константная ссылка на текущий цвет источника света.
 */
const Vec3& PointLight::GetColor() const {
    return color_;
}

/**
 * @brief Изменяет цвет PointLight.
 *
 * @param color Новый RGB-цвет источника света.
 */
void PointLight::SetColor(const Vec3& color) {
    color_ = color;
}

/**
 * @brief Возвращает интенсивность PointLight.
 *
 * @return Текущее значение интенсивности источника света.
 */
float PointLight::GetIntensity() const {
    return intensity_;
}

/**
 * @brief Изменяет интенсивность PointLight.
 *
 * Интенсивность ограничивается снизу значением 0.0f.
 *
 * Если передано отрицательное значение,
 * intensity_ устанавливается равным 0.0f.
 *
 * @param intensity Новая интенсивность источника света.
 */
void PointLight::SetIntensity(float intensity) {
    intensity_ = std::max(0.0f, intensity);
}

/**
 * @brief Проверяет, включён ли PointLight.
 *
 * Выключенный источник света может быть сохранён
 * в сцене, но его состояние определяется
 * значением enabled_.
 *
 * @return true, если PointLight включён,
 * иначе false.
 */
bool PointLight::IsEnabled() const {
    return enabled_;
}

/**
 * @brief Включает или выключает PointLight.
 *
 * @param enabled Новое состояние источника света.
 */
void PointLight::SetEnabled(bool enabled) {
    enabled_ = enabled;
}

/**
 * @brief Проверяет, должен ли PointLight отбрасывать тени.
 *
 * Флаг определяет состояние shadow rendering
 * для данного источника света.
 *
 * @return true, если построение теней включено,
 * иначе false.
 */
bool PointLight::CastsShadows() const {
    return cast_shadows_;
}

/**
 * @brief Включает или выключает построение теней для PointLight.
 *
 * Метод изменяет только соответствующий флаг состояния.
 *
 * @param cast_shadows Новое состояние shadow rendering.
 */
void PointLight::SetCastShadows(bool cast_shadows) {
    cast_shadows_ = cast_shadows;
}