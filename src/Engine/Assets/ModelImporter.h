#pragma once

#include "Engine/Assets/TextureManager.h"
#include "Engine/Scene/SceneObject.h"

#include <filesystem>
#include <memory>
#include <vector>

/**
 * @brief Преобразует данные импортируемой 3D-модели
 * в runtime-объекты Mini Engine.
 *
 * ModelImporter является промежуточным уровнем между
 * файловыми parser-компонентами и объектами сцены движка.
 *
 * ObjParser отвечает за чтение данных форматов OBJ и MTL
 * и формирует промежуточное представление ImportedModelData.
 *
 * ModelImporter на основе этих данных создаёт:
 *
 * - Mesh;
 * - Material;
 * - Texture2D;
 * - SceneObject;
 * - SceneRenderPart.
 *
 * Один OBJ-файл представлен одним SceneObject.
 *
 * Если модель использует несколько материалов через usemtl,
 * её геометрия разделяется на несколько SceneRenderPart.
 * Каждый SceneRenderPart содержит собственные Mesh и Material,
 * но все части принадлежат одному SceneObject и используют
 * общий Transform объекта.
 *
 * ModelImporter не хранит состояние между вызовами.
 */
class ModelImporter {
public:
    /**
     * @brief Импортирует модель формата Wavefront OBJ.
     *
     * Метод выполняет полный путь преобразования модели:
     *
     * OBJ / MTL
     * -> ImportedModelData
     * -> Mesh
     * -> Material
     * -> Texture2D
     * -> SceneObject.
     *
     * Если OBJ использует несколько материалов,
     * каждому набору геометрии с отдельным материалом
     * соответствует собственный SceneRenderPart.
     *
     * При этом все части остаются внутри одного SceneObject,
     * благодаря чему Position, Rotation и Scale применяются
     * ко всей импортированной модели целиком.
     *
     * Если чтение OBJ завершилось ошибкой либо после обработки
     * не осталось пригодной для рендеринга геометрии,
     * возвращается пустой список.
     *
     * @param path Путь к OBJ-файлу.
     * @param texture_manager Менеджер, используемый для загрузки Texture2D.
     *
     * @return Список созданных объектов сцены.
     * В текущей реализации один успешно импортированный OBJ
     * создаёт один SceneObject.
     */
    static std::vector<std::shared_ptr<SceneObject>> ImportObj(const std::filesystem::path& path, TextureManager& texture_manager);
};