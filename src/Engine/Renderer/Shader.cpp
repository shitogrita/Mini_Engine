#include "Shader.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>


namespace {

// Читаем весь файл целиком в std::string
std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to open shader file: " + path.string()
        );
    }

    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}


// Получаем сообщение GLSL-компилятора для отдельного shader object
std::string GetShaderInfoLog(GLuint shader) {
    // Сначала узнаём размер сообщения
    GLint log_length = 0;

    glGetShaderiv(
        shader,
        GL_INFO_LOG_LENGTH,
        &log_length
    );

    if (log_length <= 0) {
        return {};
    }

    // Выделяем строку нужного размера
    std::string log(
        static_cast<std::size_t>(log_length),
        '\0'
    );

    // Сюда OpenGL запишет реальное количество символов
    GLsizei actual_length = 0;

    glGetShaderInfoLog(
        shader,
        log_length,
        &actual_length,
        log.data()
    );

    // Оставляем только реально записанную часть
    log.resize(static_cast<std::size_t>(actual_length));

    return log;
}


// Получаем сообщение linker'а для shader program
std::string GetProgramInfoLog(GLuint program) {
    // Узнаём размер сообщения linker'а
    GLint log_length = 0;

    glGetProgramiv(
        program,
        GL_INFO_LOG_LENGTH,
        &log_length
    );

    if (log_length <= 0) {
        return {};
    }

    // Выделяем строку нужного размера
    std::string log(
        static_cast<std::size_t>(log_length),
        '\0'
    );

    GLsizei actual_length = 0;

    glGetProgramInfoLog(
        program,
        log_length,
        &actual_length,
        log.data()
    );

    // Оставляем только реально записанную часть
    log.resize(static_cast<std::size_t>(actual_length));

    return log;
}

} // namespace


Shader::Shader(
    const std::filesystem::path& vertex_path,
    const std::filesystem::path& fragment_path
) {
    // Читаем GLSL-код из файлов
    std::string vertex_source = ReadFile(vertex_path);
    std::string fragment_source = ReadFile(fragment_path);


    // VERTEX SHADER

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    const char* vertex_source_ptr = vertex_source.c_str();

    glShaderSource(
        vertex_shader,
        1,
        &vertex_source_ptr,
        nullptr
    );

    glCompileShader(vertex_shader);


    // Проверяем, успешно ли скомпилировался vertex shader
    GLint vertex_success = 0;

    glGetShaderiv(
        vertex_shader,
        GL_COMPILE_STATUS,
        &vertex_success
    );

    if (vertex_success == GL_FALSE) {
        // Получаем настоящий текст ошибки ДО удаления shader object
        std::string log = GetShaderInfoLog(vertex_shader);

        glDeleteShader(vertex_shader);

        throw std::runtime_error(
            "Failed to compile vertex shader:\n" + log
        );
    }


    // FRAGMENT SHADER

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    const char* fragment_source_ptr = fragment_source.c_str();

    glShaderSource(
        fragment_shader,
        1,
        &fragment_source_ptr,
        nullptr
    );

    glCompileShader(fragment_shader);


    // Проверяем fragment shader
    GLint fragment_success = 0;

    glGetShaderiv(
        fragment_shader,
        GL_COMPILE_STATUS,
        &fragment_success
    );

    if (fragment_success == GL_FALSE) {
        // Получаем текст ошибки до glDeleteShader()
        std::string log = GetShaderInfoLog(fragment_shader);

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        throw std::runtime_error(
            "Failed to compile fragment shader:\n" + log
        );
    }


    // SHADER PROGRAM

    program_id_ = glCreateProgram();

    // Прикрепляем оба скомпилированных shader object
    // к одной shader program
    glAttachShader(program_id_, vertex_shader);
    glAttachShader(program_id_, fragment_shader);

    // Связываем vertex + fragment shader в одну программу
    glLinkProgram(program_id_);


    // Проверяем linking
    GLint link_success = 0;

    glGetProgramiv(
        program_id_,
        GL_LINK_STATUS,
        &link_success
    );

    if (link_success == GL_FALSE) {
        // Для program используется уже не GetShaderInfoLog,
        // а GetProgramInfoLog
        std::string log = GetProgramInfoLog(program_id_);

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        glDeleteProgram(program_id_);
        program_id_ = 0;

        throw std::runtime_error(
            "Failed to link shader program:\n" + log
        );
    }


    // После успешного linking отдельные shader objects
    // больше не нужны. Готовая program продолжает существовать.
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}


Shader::~Shader() {
    // program_id_ — OpenGL resource, которым владеет Shader.
    // Освобождаем его вместе с уничтожением C++ объекта.
    if (program_id_ != 0) {
        glDeleteProgram(program_id_);
        program_id_ = 0;
    }
}


void Shader::Use() const {
    // Делаем эту shader program текущей.
    // Следующие draw calls будут использовать её.
    glUseProgram(program_id_);
}


void Shader::SetInt(const std::string& name, int value) const {
    // Получаем location uniform-переменной внутри program
    GLint location =
        glGetUniformLocation(program_id_, name.c_str());

    // Передаём int в GLSL
    glUniform1i(location, value);
}


void Shader::SetFloat(const std::string& name, float value) const {
    // Получаем location uniform-переменной внутри program
    GLint location =
        glGetUniformLocation(program_id_, name.c_str());

    // Передаём float в GLSL
    glUniform1f(location, value);
}

void Shader::SetVec4(
    const std::string& name,
    const Vec4& value
) const {
    GLint location =
        glGetUniformLocation(program_id_, name.c_str());

    glUniform4f(
        location,
        value.x,
        value.y,
        value.z,
        value.w
    );
}

void Shader::SetMat4(
    const std::string& name,
    const Matrix4& value
) const {
    GLint location =
        glGetUniformLocation(program_id_, name.c_str());

    const std::array<float, 16> col_major =
        AffineTransformation::GetColMajor(value);

    glUniformMatrix4fv(
        location,
        1,
        GL_FALSE,
        col_major.data()
    );
}