#pragma once

#include "Engine/Math/matrix_types.h"

#include <filesystem>
#include <string>


class Shader {
public:
	Shader(
		const std::filesystem::path& vertex_path,
		const std::filesystem::path& fragment_path
	);

	~Shader();

	// OpenGL program нельзя копировать:
	// два Shader не должны владеть одним program_id_.
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	// Владение OpenGL program можно передавать.
	Shader(Shader&& other) noexcept;
	Shader& operator=(Shader&& other) noexcept;

	// Активировать shader program.
	void Use() const;

	// Передача uniform-переменных.
	void SetInt(
		const std::string& name,
		int value
	) const;

	void SetFloat(
		const std::string& name,
		float value
	) const;

	void SetVec4(
		const std::string& name,
		const Vec4& value
	) const;

	void SetMat4(
		const std::string& name,
		const Matrix4& value
	) const;

private:
	// OpenGL ID слинкованной shader program.
	unsigned int program_id_ = 0;
};