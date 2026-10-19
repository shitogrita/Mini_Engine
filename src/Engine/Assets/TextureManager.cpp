#include "Engine/Assets/TextureManager.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

/**
 * @brief Загружает Texture2D с диска или возвращает её из cache.
 *
 * Метод реализует централизованную загрузку текстур Mini Engine.
 *
 * Общая последовательность работы:
 *
 * 1. Проверяется входной путь.
 * 2. Путь нормализуется.
 * 3. Проверяется существование файла.
 * 4. Проверяется, что путь соответствует обычному файлу.
 * 5. Выполняется поиск Texture2D во внутреннем cache.
 * 6. Если texture отсутствует, создаётся новый Texture2D.
 * 7. Созданный ресурс сохраняется в cache.
 *
 * Таким образом, несколько Material могут использовать
 * один и тот же экземпляр Texture2D через std::shared_ptr.
 *
 * Это предотвращает повторное:
 *
 * - чтение одного изображения с диска;
 * - декодирование изображения;
 * - создание OpenGL texture object.
 *
 * Ошибка загрузки отдельной текстуры не считается
 * критической ошибкой всего Editor.
 *
 * Если Texture2D не удалось создать,
 * исключение перехватывается, информация выводится в std::cerr,
 * а метод возвращает nullptr.
 *
 * @param path Путь к файлу изображения.
 *
 * @return Загруженная или ранее закешированная Texture2D.
 * При ошибке возвращается nullptr.
 */
std::shared_ptr<Texture2D> TextureManager::Load(const std::filesystem::path& path) {
    /**
     * Пустой путь не содержит информации
     * о ресурсе и не может быть использован для загрузки.
     */
    if (path.empty()) {
        return nullptr;
    }

    /**
     * Приводим путь к лексически нормализованной форме.
     *
     * Это удаляет избыточные элементы пути,
     * например "." и корректно обрабатываемые "..".
     *
     * Полученный путь также используется
     * для формирования ключа cache.
     */
    const std::filesystem::path normalized_path =
        path.lexically_normal();

    /**
     * До создания Texture2D проверяем,
     * существует ли указанный путь.
     *
     * Это позволяет вывести более понятное диагностическое
     * сообщение до попытки декодирования изображения.
     */
    if (!std::filesystem::exists(normalized_path)) {
        std::cerr
            << "[TextureManager] Texture file does not exist: "
            << normalized_path
            << '\n';
    }

    /**
     * Texture2D должна загружаться только
     * из обычного файла.
     *
     * Если путь отсутствует либо указывает,
     * например, на директорию, загрузка прекращается.
     */
    if (!std::filesystem::is_regular_file(normalized_path)) {
        std::cerr
            << "[TextureManager] Texture path is not a regular file: "
            << normalized_path
            << '\n';

        return nullptr;
    }

    /**
     * Строковое представление нормализованного пути
     * используется как ключ cache.
     */
    const std::string key =
        normalized_path.string();

    /**
     * Проверяем, была ли эта Texture2D загружена ранее.
     *
     * Если ресурс уже находится в cache,
     * возвращаем существующий shared_ptr
     * без повторного создания OpenGL texture.
     */
    const auto iterator =
        textures_.find(key);

    if (iterator != textures_.end()) {
        return iterator->second;
    }

    /**
     * Создание Texture2D может завершиться исключением,
     * например если изображение невозможно декодировать.
     *
     * Ошибка отдельной texture не должна завершать
     * работу всего Editor, поэтому исключение
     * обрабатывается непосредственно в TextureManager.
     */
    try {
        std::shared_ptr<Texture2D> texture =
            std::make_shared<Texture2D>(
                normalized_path
            );

        /**
         * После успешного создания ресурс добавляется
         * во внутренний cache.
         *
         * TextureManager сохраняет собственный shared_ptr,
         * поэтому texture остаётся доступной для последующих
         * вызовов Load() с тем же ключом.
         */
        textures_.emplace(
            key,
            texture
        );

        return texture;
    } catch (const std::exception& exception) {
        /**
         * Ошибка создания Texture2D считается локальной
         * ошибкой ресурса.
         *
         * Выводим путь и причину ошибки,
         * после чего возвращаем nullptr.
         */
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
 * @brief Очищает cache загруженных Texture2D.
 *
 * Метод удаляет принадлежащие TextureManager
 * std::shared_ptr из контейнера textures_.
 *
 * Если Texture2D используется другими объектами,
 * например Material, соответствующий OpenGL-ресурс
 * продолжает существовать.
 *
 * Texture2D уничтожается только после удаления
 * последнего std::shared_ptr, владеющего этим объектом.
 */
void TextureManager::Clear() {
    textures_.clear();
}