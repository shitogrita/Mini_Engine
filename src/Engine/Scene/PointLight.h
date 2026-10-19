#pragma once

#include "Engine/Math/matrix_types.h"

/**
 * @brief Представляет точечный источник света сцены.
 *
 * PointLight хранит параметры,
 * необходимые для расчёта освещения:
 *
 * - позицию источника в World Space;
 * - цвет света;
 * - интенсивность;
 * - состояние включения;
 * - состояние построения теней.
 *
 * В текущей архитектуре фактическое положение
 * PointLight при рендеринге сцены берётся
 * из Transform соответствующего SceneObject.
 *
 * Поле position_ пока сохраняется
 * для совместимости с существующим кодом.
 */
class PointLight {
public:
    /**
     * @brief Возвращает сохранённую позицию источника света.
     *
     * @return Константная ссылка на position_.
     */
    const Vec3& GetPosition() const;

    /**
     * @brief Изменяет сохранённую позицию источника света.
     *
     * @param position Новая позиция в World Space.
     */
    void SetPosition(const Vec3& position);

    /**
     * @brief Возвращает цвет источника света.
     *
     * Цвет представлен Vec3,
     * содержащим RGB-компоненты.
     *
     * @return Константная ссылка на текущий RGB-цвет.
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
     * @return Текущее значение intensity_.
     */
    float GetIntensity() const;

    /**
     * @brief Изменяет интенсивность источника света.
     *
     * Отрицательные значения автоматически
     * ограничиваются значением 0.0f.
     *
     * @param intensity Новая интенсивность.
     */
    void SetIntensity(float intensity);

    /**
     * @brief Проверяет, включён ли источник света.
     *
     * @return true, если PointLight включён,
     * иначе false.
     */
    bool IsEnabled() const;

    /**
     * @brief Включает или выключает источник света.
     *
     * @param enabled Новое состояние источника.
     */
    void SetEnabled(bool enabled);

    /**
     * @brief Проверяет, включено ли построение теней для PointLight.
     *
     * Значение является флагом состояния
     * и само по себе не создаёт shadow map.
     *
     * @return true, если для источника включены тени,
     * иначе false.
     */
    bool CastsShadows() const;

    /**
     * @brief Включает или выключает построение теней для PointLight.
     *
     * Метод изменяет только состояние cast_shadows_.
     *
     * @param cast_shadows Новое состояние shadow rendering.
     */
    void SetCastShadows(bool cast_shadows);

private:
    /**
     * @brief Сохранённая позиция источника света в World Space.
     *
     * Поле пока сохраняется для совместимости
     * с существующим кодом.
     *
     * Фактическая позиция SceneObject
     * также хранится в его Transform.
     *
     * Начальное значение:
     *
     * (2, 3, 2).
     */
    Vec3 position_{
        2.0f,
        3.0f,
        2.0f
    };

    /**
     * @brief Цвет источника света.
     *
     * По умолчанию используется белый цвет:
     *
     * (1, 1, 1).
     */
    Vec3 color_{
        1.0f,
        1.0f,
        1.0f
    };

    /**
     * @brief Множитель интенсивности света.
     *
     * По умолчанию равен 1.0f.
     */
    float intensity_ = 1.0f;

    /**
     * @brief Определяет, включён ли PointLight.
     *
     * По умолчанию источник света включён.
     */
    bool enabled_ = true;

    /**
     * @brief Определяет, включено ли построение теней.
     *
     * По умолчанию shadow rendering включён.
     */
    bool cast_shadows_ = true;
};