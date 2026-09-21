#pragma once

#include "Engine/Renderer/Texture2D.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @brief Менеджер загрузки и хранения текстур.
 *
 * TextureManager хранит уже загруженные Texture2D и не позволяет
 * повторно загружать одну и ту же текстуру в OpenGL.
 *
 * В качестве ключа используется абсолютный нормализованный путь к файлу.
 *
 * В отличие от weak_ptr, shared_ptr здесь используется намеренно:
 * менеджер владеет загруженными текстурами и контролирует время их жизни.
 * Это особенно важно для OpenGL-ресурсов, которые должны уничтожаться
 * при существующем OpenGL-контексте.
 */
class TextureManager {
public:
	/**
	 * @brief Загружает текстуру или возвращает уже загруженную.
	 *
	 * @return shared_ptr на Texture2D.
	 */
	std::shared_ptr<Texture2D> Load(const std::filesystem::path& path);

	/**
	 * @brief Удаляет все текстуры из кеша менеджера.
	 *
	 * После очистки Texture2D уничтожится, если на неё больше
	 * не существует других shared_ptr.
	 */
	void Clear();

private:
	/**
	 * Менеджер сам владеет текстурами через shared_ptr.
	 *
	 * Ключ — абсолютный нормализованный путь.
	 * Значение — загруженная Texture2D.
	 */
	std::unordered_map<std::string, std::shared_ptr<Texture2D>> textures_;
};