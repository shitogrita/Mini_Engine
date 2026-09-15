#include "TextureManager.h"

#include "Engine/Assets/TextureManager.h"
#include "Engine/Renderer/Texture2D.h"

// по факту знает о текстуре, но не нуждается в ее владении (уже владеет textures2D) Weak-ptr
std::shared_ptr<Texture2D> TextureManager::Load(const std::filesystem::path &path) {
	const std::string key = std::filesystem::absolute(path).lexically_normal().string(); // делает относительный путь абсолютным.
	// условно из Textures/brick.jpg в /Users/name/repo/Mini_Engine/Textures/brick.jpg
	// для стабильного ключа
	const auto iterator = textures_.find(key);
	if ( iterator != texture_.end()) {
		if (std::shared_ptr<Texture2D> texture = iterator->second.lock()) { // weak_ptr нельзя просто так взять объект
			// при уничтожении получим nullptr
			return texture;
		}
		textures_.erase(iterator); // очистка от мусора
	}
	auto texture = std::make_shared<Texture2D>(path);

	textures_[key] = texture;

	return texture;
}

void TextureManager::Clear() {
	textures_.clear();
}