#pragma once

#include "Engine/Assets/TextureManager.h"
#include "Engine/Scene/SceneObject.h"

#include <filesystem>
#include <memory>
#include <vector>

/**
 * @brief Импортирует модели с диска и преобразует их
 * в объекты, используемые движком.
 *
 * ObjParser отвечает только за чтение OBJ и MTL.
 * ModelImporter преобразует ImportedModelData
 * в Mesh, Material, Texture2D и SceneObject.
 */
class ModelImporter {
public:
	/**
	 * @brief Импортирует OBJ-модель.
	 *
	 * Один OBJ может использовать несколько материалов.
	 * Поэтому один файл может создать несколько SceneObject:
	 * по одному объекту для каждой части модели с отдельным материалом.
	 *
	 * @param path Путь к OBJ-файлу.
	 * @param texture_manager Менеджер текстур движка.
	 * @return Созданные объекты сцены.
	 */
	static std::vector<std::shared_ptr<SceneObject>> ImportObj(
		const std::filesystem::path& path,
		TextureManager& texture_manager);
};