#include "Engine/Renderer/Material.h"

#include <utility>

/**
 * @brief Возвращает основной цвет материала.
 *
 * @return Константная ссылка на текущий цвет материала.
 */
const Vec3& Material::GetColor() const {
    return color_;
}

/**
 * @brief Устанавливает основной цвет материала.
 *
 * @param color Новое значение цвета.
 */
void Material::SetColor(const Vec3& color) {
    color_ = color;
}

/**
 * @brief Возвращает коэффициент ambient-составляющей материала.
 *
 * @return Текущее значение ambient strength.
 */
float Material::GetAmbientStrength() const {
    return ambient_strength_;
}

/**
 * @brief Устанавливает коэффициент ambient-составляющей материала.
 *
 * @param value Новое значение ambient strength.
 */
void Material::SetAmbientStrength(float value) {
    ambient_strength_ = value;
}

/**
 * @brief Возвращает коэффициент diffuse-составляющей материала.
 *
 * @return Текущее значение diffuse strength.
 */
float Material::GetDiffuseStrength() const {
    return diffuse_strength_;
}

/**
 * @brief Устанавливает коэффициент diffuse-составляющей материала.
 *
 * @param value Новое значение diffuse strength.
 */
void Material::SetDiffuseStrength(float value) {
    diffuse_strength_ = value;
}

/**
 * @brief Возвращает коэффициент specular-составляющей материала.
 *
 * @return Текущее значение specular strength.
 */
float Material::GetSpecularStrength() const {
    return specular_strength_;
}

/**
 * @brief Устанавливает коэффициент specular-составляющей материала.
 *
 * @param value Новое значение specular strength.
 */
void Material::SetSpecularStrength(float value) {
    specular_strength_ = value;
}

/**
 * @brief Возвращает коэффициент блеска материала.
 *
 * Значение shininess определяет параметр,
 * используемый для характеристики specular-составляющей материала.
 *
 * @return Текущее значение shininess.
 */
float Material::GetShininess() const {
    return shininess_;
}

/**
 * @brief Устанавливает коэффициент блеска материала.
 *
 * @param value Новое значение shininess.
 */
void Material::SetShininess(float value) {
    shininess_ = value;
}

/**
 * @brief Назначает diffuse-текстуру материалу.
 *
 * Материал хранит текстуру через std::shared_ptr,
 * поэтому ресурс может совместно использоваться
 * несколькими объектами или материалами.
 *
 * Переданный shared_ptr перемещается
 * во внутреннее поле diffuse_texture_.
 *
 * Метод изменяет только объект Texture2D
 * и не изменяет diffuse_texture_path_.
 *
 * @param texture Текстура, назначаемая материалу.
 */
void Material::SetDiffuseTexture(std::shared_ptr<Texture2D> texture) {
    diffuse_texture_ = std::move(texture);
}

/**
 * @brief Возвращает diffuse-текстуру материала.
 *
 * Возвращается константная ссылка на std::shared_ptr,
 * что позволяет получить доступ к Texture2D
 * без копирования самого shared_ptr.
 *
 * @return Константная ссылка на diffuse_texture_.
 */
const std::shared_ptr<Texture2D>& Material::GetDiffuseTexture() const {
    return diffuse_texture_;
}

/**
 * @brief Проверяет наличие diffuse-текстуры.
 *
 * Материал считается имеющим diffuse-текстуру,
 * если diffuse_texture_ содержит ненулевой указатель.
 *
 * @return true, если diffuse-текстура назначена;
 * иначе false.
 */
bool Material::HasDiffuseTexture() const {
    return diffuse_texture_ != nullptr;
}

/**
 * @brief Удаляет diffuse-текстуру из материала.
 *
 * Метод:
 *
 * - сбрасывает std::shared_ptr<Texture2D>;
 * - очищает сохранённый путь к diffuse-текстуре.
 *
 * После вызова HasDiffuseTexture() возвращает false.
 */
void Material::ClearDiffuseTexture() {
    diffuse_texture_.reset();
    diffuse_texture_path_.clear();
}

/**
 * @brief Сохраняет путь к diffuse-текстуре материала.
 *
 * Путь хранится отдельно от объекта Texture2D
 * и может использоваться для сохранения
 * или восстановления информации о материале.
 *
 * Метод изменяет только diffuse_texture_path_
 * и не изменяет diffuse_texture_.
 *
 * @param path Путь к diffuse-текстуре.
 */
void Material::SetDiffuseTexturePath(std::filesystem::path path) {
    diffuse_texture_path_ = std::move(path);
}

/**
 * @brief Возвращает сохранённый путь к diffuse-текстуре.
 *
 * @return Константная ссылка на diffuse_texture_path_.
 */
const std::filesystem::path& Material::GetDiffuseTexturePath() const {
    return diffuse_texture_path_;
}