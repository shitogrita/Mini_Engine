#pragma once

#include <filesystem>
#include <string>

class Shader {
  public:
	Shader(
		const std::filesystem::path& vertex_path,
		const std::filesystem::path& fragment_path
	);

	~Shader();

	// Нельзя копировать OpenGL
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	Shader(Shader&& other) noexcept;
	Shader& operator=(Shader&& other) noexcept;

	// Активировать shader program
	void Use() const;

	void SetFloat(const std::string& name, float value) const;
	void SetInt(const std::string& name, int value) const;

	// Передать uniform (скоро после проверки своего matrix)
	void SetVec4(const std::string& name, const Vec4& value) const;
	void SetMat4(
		const std::string& name,
		const Matrix4& value
	) const;
	void SetFloat(const std::string& name, float value) const;
	void SetInt(const std::string& name, int value) const
	// void SetVec3(/* имя uniform */, /* значение */);


  private:
	// OpenGL ID слинкованной shader program
	unsigned int program_id_ = 0;
};