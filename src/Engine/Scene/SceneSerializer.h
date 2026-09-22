#pragma once

#include "Engine/Scene/Scene.h"

#include <filesystem>

class TextureManager;

class SceneSerializer {
public:
	static bool Save(const Scene& scene, const std::filesystem::path& path);
	static bool Load(Scene& scene, const std::filesystem::path& path, TextureManager& texture_manager);
};