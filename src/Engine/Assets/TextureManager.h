#pragma once

#include "Engine/Renderer/Texture2D.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @brief Управляет загрузкой и хранением текстур Mini Engine.
 *
 * TextureManager выполняет две основные задачи:
 *
 * - загружает Texture2D с диска;
 * - повторно использует уже созданные текстуры через внутренний cache.
 *
 * Это позволяет избежать повторного чтения одного и того же изображения
 * и повторного создания соответствующего OpenGL texture object,
 * если одна Texture2D используется несколькими Material.
 *
 * В качестве ключа cache используется строковое представление
 * нормализованного пути к файлу.
 *
 * Менеджер хранит Texture2D через std::shared_ptr.
 * Благодаря этому TextureManager владеет загруженными ресурсами,
 * пока они находятся во внутреннем cache.
 *
 * При очистке cache объект Texture2D уничтожается только тогда,
 * когда на него больше не существует других std::shared_ptr.
 *
 * TextureManager не занимается:
 *
 * - декодированием форматов изображений;
 * - настройкой Material;
 * - привязкой texture к shader;
 * - отрисовкой объектов.
 *
 * Непосредственное создание OpenGL texture выполняется Texture2D.
 */
class TextureManager {
public:
    /**
     * @brief Загружает Texture2D с диска или возвращает ранее загруженную.
     *
     * Перед загрузкой путь нормализуется и используется
     * как ключ внутреннего cache.
     *
     * Если соответствующая Texture2D уже присутствует в cache,
     * повторная загрузка изображения не выполняется.
     *
     * Если файл отсутствует, не является обычным файлом
     * либо Texture2D не удалось создать,
     * метод возвращает nullptr.
     *
     * Ошибка загрузки отдельной текстуры
     * не приводит к завершению работы приложения.
     *
     * @param path Путь к файлу изображения.
     *
     * @return std::shared_ptr на загруженную Texture2D
     * или nullptr при ошибке загрузки.
     */
    std::shared_ptr<Texture2D> Load(const std::filesystem::path& path);

    /**
     * @brief Очищает внутренний cache TextureManager.
     *
     * Все std::shared_ptr, принадлежащие TextureManager,
     * удаляются из контейнера.
     *
     * Сам объект Texture2D уничтожается только в том случае,
     * если после очистки cache на него больше
     * не существует других std::shared_ptr.
     */
    void Clear();

private:
    /**
     * @brief Cache уже загруженных текстур.
     *
     * Ключ:
     *
     * строковое представление нормализованного пути к изображению.
     *
     * Значение:
     *
     * std::shared_ptr на соответствующую Texture2D.
     *
     * Хранение shared_ptr позволяет TextureManager
     * сохранять владение загруженными текстурами
     * до вызова Clear() или уничтожения самого менеджера.
     */
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> textures_;
};