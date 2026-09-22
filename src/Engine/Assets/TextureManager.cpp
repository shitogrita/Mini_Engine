#include "Engine/Assets/TextureManager.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

/**
 * @brief Загружает текстуру с диска или возвращает уже загруженную.
 *
 * TextureManager используется как cache:
 *
 * path
 * -> Texture2D
 *
 * Если одна и та же текстура используется несколькими материалами,
 * повторного чтения изображения и создания OpenGL texture не происходит.
 *
 * Ошибка загрузки отдельной текстуры не считается причиной
 * для завершения всего приложения. В этом случае метод возвращает nullptr,
 * а модель продолжает загружаться без этой текстуры.
 *
 * @param path Путь к изображению.
 *
 * @return Загруженная Texture2D или nullptr при ошибке.
 */
std::shared_ptr<Texture2D> TextureManager::Load(const std::filesystem::path& path) {
    if (path.empty()) {
        return nullptr;
    }

    const std::filesystem::path normalized_path =
        path.lexically_normal();

    /*
     * До вызова stb_image проверяем существование файла.
     *
     * Это позволяет отличить:
     *
     * - неправильный путь;
     * - существующий, но неподдерживаемый файл.
     */
    if (!std::filesystem::exists(normalized_path)) {
        std::cerr
            << "[TextureManager] Texture file does not exist: "
            << normalized_path
            << '\n';

    }

    if (!std::filesystem::is_regular_file(normalized_path)) {
        std::cerr
            << "[TextureManager] Texture path is not a regular file: "
            << normalized_path
            << '\n';

        return nullptr;
    }

    const std::string key =
        normalized_path.string();

    /*
     * Возвращаем texture из cache,
     * если она уже была загружена.
     */
    const auto iterator =
        textures_.find(key);

    if (iterator != textures_.end()) {
        return iterator->second;
    }

    /*
     * Texture2D может бросить исключение,
     * если stb_image не смог декодировать файл.
     *
     * Такое состояние не должно ронять Editor.
     */
    try {
        std::shared_ptr<Texture2D> texture =
            std::make_shared<Texture2D>(
                normalized_path
            );

        textures_.emplace(
            key,
            texture
        );

        return texture;
    } catch (const std::exception& exception) {
        std::cerr
            << "[TextureManager] Failed to load texture: "
            << normalized_path
            << '\n'
            << "[TextureManager] Reason: "
            << exception.what()
            << '\n';

        return nullptr;
    }
}

/**
 * @brief Очищает cache текстур.
 *
 * shared_ptr<Texture2D> удаляются после того,
 * как на них больше не остаётся ссылок.
 */
void TextureManager::Clear() {
    textures_.clear();
}