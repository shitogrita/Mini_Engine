#pragma once

#include "Engine/Math/matrix_types.h"

/**
 * @brief Точечный источник света сцены.
 *
 * PointLight хранит параметры, необходимые
 * для расчёта освещения в Renderer:
 *
 * - позицию источника в World Space;
 * - цвет света;
 * - интенсивность;
 * - состояние включения.
 *
 * Сам PointLight не содержит Mesh.
 * Сфера, отображаемая в SceneViewport,
 * является только editor-визуализацией источника.
 */
class PointLight {
public:
    /**
     * @brief Возвращает позицию источника света.
     *
     * @return Позиция в World Space.
     */
    const Vec3& GetPosition() const {
        return position_;
    }

    /**
     * @brief Изменяет позицию источника света.
     *
     * @param position Новая позиция в World Space.
     */
    void SetPosition(const Vec3& position) {
        position_ = position;
    }

    /**
     * @brief Возвращает цвет света.
     *
     * Компоненты RGB находятся в диапазоне [0, 1].
     *
     * @return Цвет источника.
     */
    const Vec3& GetColor() const {
        return color_;
    }

    /**
     * @brief Изменяет цвет света.
     *
     * @param color Новый RGB-цвет.
     */
    void SetColor(const Vec3& color) {
        color_ = color;
    }

    /**
     * @brief Возвращает интенсивность света.
     *
     * @return Текущая интенсивность.
     */
    float GetIntensity() const {
        return intensity_;
    }

    /**
     * @brief Изменяет интенсивность света.
     *
     * Отрицательная интенсивность не допускается.
     *
     * @param intensity Новая интенсивность.
     */
    void SetIntensity(float intensity) {
        intensity_ = intensity < 0.0f ? 0.0f : intensity;
    }

    /**
     * @brief Проверяет, включён ли источник света.
     *
     * @return true, если PointLight участвует в освещении.
     */
    bool IsEnabled() const {
        return enabled_;
    }

    /**
     * @brief Включает или выключает источник света.
     *
     * @param enabled Новое состояние.
     */
    void SetEnabled(bool enabled) {
        enabled_ = enabled;
    }

private:
    /**
     * @brief Позиция источника в World Space.
     */
    Vec3 position_{2.0f, 3.0f, 2.0f};

    /**
     * @brief Цвет источника.
     */
    Vec3 color_{1.0f, 1.0f, 1.0f};

    /**
     * @brief Множитель яркости света.
     */
    float intensity_ = 1.0f;

    /**
     * @brief Участвует ли источник в освещении.
     */
    bool enabled_ = true;
};