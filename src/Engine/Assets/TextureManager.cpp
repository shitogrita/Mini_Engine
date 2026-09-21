#include "Engine/Assets/TextureManager.h"

std::shared_ptr<Texture2D> TextureManager::Load(const std::filesystem::path& path) {
    /**
     * Делаем относительный путь абсолютным и нормализуем его.
     *
     * Например:
     * Textures/brick.jpg
     *
     * может превратиться в:
     * /Users/name/repo/Mini_Engine/Textures/brick.jpg
     *
     * Это необходимо для получения стабильного ключа кеша.
     * Иначе разные записи одного пути могли бы восприниматься
     * как разные текстуры.
     */
    const std::filesystem::path normalized_path =
        std::filesystem::absolute(path).lexically_normal();

    const std::string key = normalized_path.string();

    // Проверяем, загружалась ли эта текстура раньше.
    const auto iterator = textures_.find(key);

    if (iterator != textures_.end()) {
        // Текстура уже существует — повторно в OpenGL её не загружаем.
        return iterator->second;
    }

    /**
     * Текстура ещё не загружена.
     *
     * Создаём Texture2D. Texture2D внутри себя загружает изображение
     * и создаёт соответствующий OpenGL texture object.
     */
    auto texture = std::make_shared<Texture2D>(normalized_path);

    // TextureManager становится одним из владельцев Texture2D.
    textures_[key] = texture;

    return texture;
}

void TextureManager::Clear() {
    /**
     * Убираем shared_ptr менеджера на все текстуры.
     *
     * Если других владельцев Texture2D нет, вызывается её деструктор
     * и освобождается соответствующий OpenGL-ресурс.
     *
     * Поэтому Clear() необходимо вызывать, пока OpenGL-контекст
     * ещё существует.
     */
    textures_.clear();
}