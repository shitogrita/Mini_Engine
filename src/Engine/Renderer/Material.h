#pragma once

#include "Engine/Math/matrix_types.h"
#include "Engine/Renderer/Texture2D.h"

#include <filesystem>
#include <memory>

/**
 * @brief Описывает визуальные свойства поверхности объекта.
 *
 * Material не содержит геометрию объекта
 * и не является источником света.
 *
 * Класс хранит параметры,
 * которые определяют внешний вид поверхности
 * при расчёте освещения и текстурирования.
 *
 * Основные свойства материала:
 *
 * - Color — базовый цвет поверхности;
 * - AmbientStrength — вклад фонового освещения;
 * - DiffuseStrength — интенсивность diffuse-составляющей;
 * - SpecularStrength — интенсивность specular-блика;
 * - Shininess — характеристика размера и резкости блика;
 * - DiffuseTexture — diffuse-текстура материала;
 * - DiffuseTexturePath — путь к исходному файлу текстуры.
 *
 * Material хранит Texture2D через std::shared_ptr,
 * поэтому одна текстура может совместно использоваться
 * несколькими материалами или объектами.
 */
class Material {
public:
    /**
     * @brief Создаёт материал со значениями по умолчанию.
     */
    Material() = default;

    /**
     * @brief Возвращает основной цвет материала.
     *
     * @return Константная ссылка на цвет материала.
     */
    const Vec3& GetColor() const;

    /**
     * @brief Устанавливает основной цвет материала.
     *
     * @param color Новый цвет материала.
     */
    void SetColor(const Vec3& color);

    /**
     * @brief Возвращает коэффициент ambient-составляющей.
     *
     * @return Текущее значение ambient strength.
     */
    float GetAmbientStrength() const;

    /**
     * @brief Устанавливает коэффициент ambient-составляющей.
     *
     * @param value Новое значение ambient strength.
     */
    void SetAmbientStrength(float value);

    /**
     * @brief Возвращает коэффициент diffuse-составляющей.
     *
     * @return Текущее значение diffuse strength.
     */
    float GetDiffuseStrength() const;

    /**
     * @brief Устанавливает коэффициент diffuse-составляющей.
     *
     * @param value Новое значение diffuse strength.
     */
    void SetDiffuseStrength(float value);

    /**
     * @brief Возвращает коэффициент specular-составляющей.
     *
     * @return Текущее значение specular strength.
     */
    float GetSpecularStrength() const;

    /**
     * @brief Устанавливает коэффициент specular-составляющей.
     *
     * @param value Новое значение specular strength.
     */
    void SetSpecularStrength(float value);

    /**
     * @brief Возвращает коэффициент shininess.
     *
     * Значение используется для характеристики
     * specular-блика поверхности.
     *
     * @return Текущее значение shininess.
     */
    float GetShininess() const;

    /**
     * @brief Устанавливает коэффициент shininess.
     *
     * @param value Новое значение shininess.
     */
    void SetShininess(float value);

    /**
     * @brief Назначает diffuse-текстуру материалу.
     *
     * Текстура хранится через std::shared_ptr,
     * что позволяет совместно использовать
     * один ресурс Texture2D.
     *
     * Метод изменяет только diffuse_texture_
     * и не изменяет сохранённый путь к текстуре.
     *
     * @param texture Назначаемая diffuse-текстура.
     */
    void SetDiffuseTexture(std::shared_ptr<Texture2D> texture);

    /**
     * @brief Возвращает diffuse-текстуру материала.
     *
     * @return Константная ссылка на std::shared_ptr<Texture2D>.
     */
    const std::shared_ptr<Texture2D>& GetDiffuseTexture() const;

    /**
     * @brief Проверяет наличие diffuse-текстуры.
     *
     * @return true, если diffuse_texture_ содержит Texture2D;
     * иначе false.
     */
    bool HasDiffuseTexture() const;

    /**
     * @brief Удаляет diffuse-текстуру и сохранённый путь к ней.
     *
     * После вызова метода:
     *
     * - diffuse_texture_ сбрасывается;
     * - diffuse_texture_path_ очищается.
     */
    void ClearDiffuseTexture();

    /**
     * @brief Сохраняет путь к diffuse-текстуре.
     *
     * Путь хранится отдельно от объекта Texture2D.
     *
     * Метод не изменяет diffuse_texture_.
     *
     * @param path Путь к файлу diffuse-текстуры.
     */
    void SetDiffuseTexturePath(std::filesystem::path path);

    /**
     * @brief Возвращает сохранённый путь к diffuse-текстуре.
     *
     * @return Константная ссылка на diffuse_texture_path_.
     */
    const std::filesystem::path& GetDiffuseTexturePath() const;

private:
    /**
     * @brief Базовый цвет материала.
     *
     * Значение по умолчанию:
     *
     * RGB = (0.67, 0.76, 0.91).
     */
    Vec3 color_{
        0.67f,
        0.76f,
        0.91f
    };

    /**
     * @brief Коэффициент ambient-составляющей материала.
     */
    float ambient_strength_ = 0.20f;

    /**
     * @brief Коэффициент diffuse-составляющей материала.
     */
    float diffuse_strength_ = 1.00f;

    /**
     * @brief Коэффициент specular-составляющей материала.
     */
    float specular_strength_ = 0.50f;

    /**
     * @brief Коэффициент shininess материала.
     */
    float shininess_ = 32.0f;

    /**
     * @brief Загруженная diffuse-текстура материала.
     *
     * Ресурс хранится через std::shared_ptr
     * и может использоваться совместно.
     */
    std::shared_ptr<Texture2D> diffuse_texture_;

    /**
     * @brief Путь к файлу diffuse-текстуры.
     *
     * Хранится отдельно от объекта Texture2D
     * и может использоваться при сохранении
     * и восстановлении данных материала.
     */
    std::filesystem::path diffuse_texture_path_;
};