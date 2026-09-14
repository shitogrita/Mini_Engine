#pragma once

#include <filesystem>


/**
 * @brief OpenGL 2D texture.
 *
 * Загружает изображение с диска,
 * создаёт texture object в OpenGL
 * и управляет временем его жизни.
 */
class Texture2D {
public:
	explicit Texture2D(
		const std::filesystem::path& path
	);

	~Texture2D();


	Texture2D(
		const Texture2D&
	) = delete;

	Texture2D& operator=(
		const Texture2D&
	) = delete;


	Texture2D(
		Texture2D&& other
	) noexcept;

	Texture2D& operator=(
		Texture2D&& other
	) noexcept;


	/**
	 * @brief Привязывает texture
	 * к указанному texture slot.
	 *
	 * slot = 0 соответствует GL_TEXTURE0.
	 */
	void Bind(
		unsigned int slot = 0
	) const;


	unsigned int GetId() const;

	int GetWidth() const;

	int GetHeight() const;

	int GetChannels() const;


private:
	unsigned int texture_id_ = 0;

	int width_ = 0;
	int height_ = 0;
	int channels_ = 0;
};