#include "../Shaders/basic.frag"
#include "../Shaders/basic.vert"

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

	// Активировать shader program
	void Use() const;

	// Передать uniform
	void SetVec4(/* имя uniform */, /* значение */);
	void SetFloat(/* имя uniform */, float value);
	void SetInt(/* имя uniform */, int value);
	void SetVec3(/* имя uniform */, /* значение */);
	void SetMat4(/* имя uniform */, /* матрица */);;

private:
	// OpenGL ID слинкованной shader program
	unsigned int program_id_ = 0;
};