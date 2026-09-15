#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class Texture2D;

/**
 * @brief Управляет загрузкой и повторным использованием текстур.
 *
 * Если одна текстура запрашивается несколько раз,
 * TextureManager возвращает уже существующий объект,
 * а не создаёт новую OpenGL-текстуру.
 */
class TextureManager {
public:
	std::shared_ptr<Texture2D> Load(const std::filesystem::path& path);

	void Clear();

private:
	std::unordered_map<std::string, std::weak_ptr<Texture2D>> textures_;
};