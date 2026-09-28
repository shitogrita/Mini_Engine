#pragma once

#include "Engine/Math/matrix_types.h"

/**
 * @brief Точечный источник света сцены.
 *
 * PointLight хранит параметры, необходимые
 * для расчёта освещения:
 *
 * - позицию источника в World Space;
 * - цвет света;
 * - интенсивность;
 * - состояние включения;
 * - состояние построения теней.
 *
 * В текущей архитектуре фактическое положение
 * PointLight в SceneViewport берётся из Transform
 * соответствующего SceneObject.
 *
 * position_ пока сохраняется для совместимости
 * с существующим кодом и будет удалён отдельно
 * после полной проверки всех зависимостей.
 */
class PointLight {
public:
    /**
     * @brief Возвращает позицию источника света.
     *
     * @return Позиция PointLight в World Space.
     */
    const Vec3& GetPosition() const;

    /**
     * @brief Изменяет позицию источника света.
     *
     * @param position Новая позиция в World Space.
     */
    void SetPosition(const Vec3& position);

    /**
     * @brief Возвращает цвет источника света.
     *
     * @return RGB-цвет источника.
     */
    const Vec3& GetColor() const;

    /**
     * @brief Изменяет цвет источника света.
     *
     * @param color Новый RGB-цвет.
     */
    void SetColor(const Vec3& color);

    /**
     * @brief Возвращает интенсивность источника света.
     *
     * @return Текущая интенсивность.
     */
    float GetIntensity() const;

    /**
     * @brief Изменяет интенсивность источника света.
     *
     * Отрицательные значения автоматически
     * ограничиваются нулём.
     *
     * @param intensity Новая интенсивность.
     */
    void SetIntensity(float intensity);

    /**
     * @brief Проверяет, включён ли источник света.
     *
     * @return true, если источник участвует в освещении.
     */
    bool IsEnabled() const;

    /**
     * @brief Включает или выключает источник света.
     *
     * @param enabled Новое состояние источника.
     */
    void SetEnabled(bool enabled);

    /**
     * @brief Проверяет, должен ли PointLight отбрасывать тени.
     *
     * Само значение не создаёт shadow map автоматически.
     * Оно только сообщает Renderer, нужно ли строить
     * Depth Cubemap для данного источника.
     *
     * @return true, если источник должен отбрасывать тени.
     */
    bool CastsShadows() const;

    /**
     * @brief Включает или выключает тени PointLight.
     *
     * @param cast_shadows Новое состояние shadow rendering.
     */
    void SetCastShadows(bool cast_shadows);

private:
    /**
     * @brief Позиция источника света в World Space.
     *
     * Поле пока сохраняется для совместимости.
     * Фактическая позиция SceneObject уже хранится
     * в Transform.
     */
    Vec3 position_{2.0f, 3.0f, 2.0f};

    /**
     * @brief Цвет источника света.
     */
    Vec3 color_{1.0f, 1.0f, 1.0f};

    /**
     * @brief Множитель яркости света.
     */
    float intensity_ = 1.0f;

    /**
     * @brief Участвует ли PointLight в освещении.
     */
    bool enabled_ = true;

    /**
     * @brief Нужно ли источнику строить тени.
     *
     * По умолчанию включено.
     */
    bool cast_shadows_ = true;
};