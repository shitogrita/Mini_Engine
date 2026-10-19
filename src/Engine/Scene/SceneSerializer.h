#pragma once

#include "Engine/Scene/Scene.h"

#include <filesystem>

class TextureManager;

/**
 * @brief Выполняет сериализацию и восстановление Scene.
 *
 * SceneSerializer сохраняет структуру сцены
 * в текстовом формате Mini Engine
 * и восстанавливает runtime SceneObject.
 *
 * Сериализация включает:
 *
 * - имя объекта;
 * - тип SceneObject;
 * - Transform;
 * - путь к исходному ресурсу;
 * - основной Material;
 * - материалы multipart-модели;
 * - пути к diffuse texture;
 * - компонент PointLight.
 *
 * Геометрия примитивов непосредственно
 * в файл сцены не сохраняется:
 * при загрузке она повторно создаётся
 * через PrimitiveGenerator.
 *
 * Импортированные модели восстанавливаются
 * повторным импортом исходного файла,
 * путь к которому хранится в SceneObject.
 *
 * Текущая версия формата — 4.
 *
 * Version 4 добавляет сериализацию параметров PointLight:
 *
 * - color;
 * - intensity;
 * - enabled.
 *
 * Положение источника света хранится
 * в Transform соответствующего SceneObject.
 *
 * Загрузчик также поддерживает format version 3
 * для совместимости с ранее сохранёнными сценами.
 */
class SceneSerializer {
public:
    /**
     * @brief Сохраняет Scene в файл.
     *
     * Сцена сериализуется в текстовый формат Mini Engine.
     *
     * Для каждого корректного SceneObject сохраняются
     * его основные свойства, Transform,
     * материалы и дополнительные данные,
     * необходимые для последующего восстановления.
     *
     * nullptr-объекты внутри Scene
     * не записываются в файл.
     *
     * @param scene Сцена, которую необходимо сохранить.
     * @param path Путь к файлу сцены.
     *
     * @return true, если сохранение завершилось успешно.
     * @return false, если файл невозможно открыть
     * или объект не может быть корректно сериализован.
     */
    static bool Save(const Scene& scene, const std::filesystem::path& path);

    /**
     * @brief Загружает Scene из файла.
     *
     * Поддерживаются форматы:
     *
     * - version 3;
     * - version 4.
     *
     * Version 4 восстанавливает сохранённые параметры PointLight:
     *
     * - color;
     * - intensity;
     * - enabled.
     *
     * При загрузке сначала считываются
     * и создаются все объекты новой сцены.
     *
     * Существующая Scene очищается
     * только после успешного восстановления
     * всех SceneObject.
     *
     * TextureManager используется
     * для загрузки diffuse texture
     * и восстановления импортированных моделей.
     *
     * @param scene Сцена, содержимое которой
     * будет заменено загруженными объектами.
     * @param path Путь к файлу сцены.
     * @param texture_manager Менеджер загрузки
     * и кеширования Texture.
     *
     * @return true, если файл корректно прочитан
     * и Scene успешно восстановлена.
     * @return false при ошибке чтения,
     * неизвестной версии формата
     * или невозможности создать один из объектов.
     */
    static bool Load(Scene& scene, const std::filesystem::path& path, TextureManager& texture_manager);
};