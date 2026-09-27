#pragma once

#include "Engine/Scene/Scene.h"

#include <filesystem>

class TextureManager;

/**
 * @brief Сериализация и восстановление Scene.
 *
 * SceneSerializer сохраняет структуру сцены в текстовый формат
 * Mini Engine и восстанавливает SceneObject вместе с:
 * - Transform;
 * - типом объекта;
 * - Material;
 * - multipart-материалами импортированной модели;
 * - путями к Texture;
 * - компонентом PointLight.
 *
 * Версия формата 4 добавляет полноценное сохранение PointLight.
 */
class SceneSerializer {
public:
	/**
	 * @brief Сохраняет Scene в файл.
	 *
	 * @param scene Сцена, которую необходимо сохранить.
	 * @param path Путь к файлу сцены.
	 * @return true, если сохранение завершилось успешно.
	 */
	static bool Save(const Scene& scene, const std::filesystem::path& path);

	/**
	 * @brief Загружает Scene из файла.
	 *
	 * Поддерживаются форматы версии 3 и 4.
	 * Version 4 восстанавливает параметры PointLight полностью.
	 *
	 * @param scene Сцена, которая будет заменена загруженными объектами.
	 * @param path Путь к файлу сцены.
	 * @param texture_manager Менеджер загрузки и кеширования Texture.
	 * @return true, если файл корректно прочитан и Scene восстановлена.
	 */
	static bool Load(Scene& scene, const std::filesystem::path& path, TextureManager& texture_manager);
};
