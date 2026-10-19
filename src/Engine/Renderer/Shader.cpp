#include "Shader.h"

#include "../Platform/OpenGL/Glad/glad.h"
#include "../Math/affine_transformation.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

/**
 * @brief Читает содержимое файла целиком в std::string.
 *
 * Функция используется для загрузки исходного GLSL-кода
 * vertex и fragment shader перед их компиляцией.
 *
 * Если файл невозможно открыть,
 * выбрасывается std::runtime_error
 * с указанием пути к проблемному файлу.
 *
 * @param path Путь к shader-файлу.
 *
 * @return Полное содержимое файла.
 *
 * @throws std::runtime_error Если файл не удалось открыть.
 */
std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to open shader file: " + path.string()
        );
    }

    /**
     * std::istreambuf_iterator позволяет считать
     * весь оставшийся поток непосредственно в std::string.
     */
    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

/**
 * @brief Получает диагностическое сообщение
 * компилятора отдельного OpenGL shader object.
 *
 * Сначала определяется требуемый размер log buffer,
 * после чего OpenGL записывает сообщение
 * через glGetShaderInfoLog().
 *
 * Итоговая строка уменьшается до фактического
 * количества записанных символов.
 *
 * @param shader OpenGL ID shader object.
 *
 * @return Сообщение GLSL-компилятора
 * или пустая строка, если log отсутствует.
 */
std::string GetShaderInfoLog(GLuint shader) {
    GLint log_length = 0;

    /**
     * Получаем размер буфера,
     * необходимого для shader info log.
     */
    glGetShaderiv(
        shader,
        GL_INFO_LOG_LENGTH,
        &log_length
    );

    if (log_length <= 0) {
        return {};
    }

    std::string log(
        static_cast<std::size_t>(log_length),
        '\0'
    );

    GLsizei actual_length = 0;

    /**
     * Получаем фактический текст сообщения
     * от GLSL-компилятора.
     */
    glGetShaderInfoLog(
        shader,
        log_length,
        &actual_length,
        log.data()
    );

    /**
     * Оставляем только реально записанную
     * OpenGL часть строки.
     */
    log.resize(
        static_cast<std::size_t>(actual_length)
    );

    return log;
}

/**
 * @brief Получает диагностическое сообщение linker'а
 * OpenGL shader program.
 *
 * В отличие от отдельного shader object,
 * для готовой program используется glGetProgramInfoLog().
 *
 * @param program OpenGL ID shader program.
 *
 * @return Сообщение linker'а
 * или пустая строка, если log отсутствует.
 */
std::string GetProgramInfoLog(GLuint program) {
    GLint log_length = 0;

    /**
     * Получаем требуемый размер program info log.
     */
    glGetProgramiv(
        program,
        GL_INFO_LOG_LENGTH,
        &log_length
    );

    if (log_length <= 0) {
        return {};
    }

    std::string log(
        static_cast<std::size_t>(log_length),
        '\0'
    );

    GLsizei actual_length = 0;

    /**
     * Получаем сообщение linker'а.
     */
    glGetProgramInfoLog(
        program,
        log_length,
        &actual_length,
        log.data()
    );

    log.resize(
        static_cast<std::size_t>(actual_length)
    );

    return log;
}

} // namespace

/**
 * @brief Загружает, компилирует и связывает GLSL shaders.
 *
 * Конструктор выполняет полный цикл создания shader program:
 *
 * 1. Загружает vertex shader из файла.
 * 2. Загружает fragment shader из файла.
 * 3. Создаёт и компилирует GL_VERTEX_SHADER.
 * 4. Проверяет результат его компиляции.
 * 5. Создаёт и компилирует GL_FRAGMENT_SHADER.
 * 6. Проверяет результат его компиляции.
 * 7. Создаёт OpenGL shader program.
 * 8. Подключает оба shader object.
 * 9. Выполняет linking.
 * 10. Проверяет результат linking.
 * 11. Удаляет отдельные shader objects после успешной линковки.
 *
 * При ошибке компиляции или linking диагностическое сообщение OpenGL
 * включается в std::runtime_error.
 *
 * Для выполнения конструктора необходим
 * активный OpenGL-контекст.
 *
 * @param vertex_path Путь к vertex shader.
 * @param fragment_path Путь к fragment shader.
 *
 * @throws std::runtime_error Если shader-файл не удалось открыть,
 * shader не скомпилировался или program не удалось связать.
 */
Shader::Shader(const std::filesystem::path& vertex_path, const std::filesystem::path& fragment_path) {
    /**
     * Загружаем GLSL source code из файлов.
     */
    std::string vertex_source = ReadFile(vertex_path);
    std::string fragment_source = ReadFile(fragment_path);

    /**
     * Создаём vertex shader object.
     */
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    const char* vertex_source_ptr = vertex_source.c_str();

    glShaderSource(
        vertex_shader,
        1,
        &vertex_source_ptr,
        nullptr
    );

    glCompileShader(vertex_shader);

    /**
     * Проверяем результат компиляции vertex shader.
     */
    GLint vertex_success = 0;

    glGetShaderiv(
        vertex_shader,
        GL_COMPILE_STATUS,
        &vertex_success
    );

    if (vertex_success == GL_FALSE) {
        /**
         * Получаем info log до удаления shader object,
         * поскольку после удаления диагностическая информация
         * больше не должна использоваться через этот handle.
         */
        std::string log = GetShaderInfoLog(vertex_shader);

        glDeleteShader(vertex_shader);

        throw std::runtime_error(
            "Failed to compile vertex shader:\n" + log
        );
    }

    /**
     * Создаём fragment shader object.
     */
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    const char* fragment_source_ptr = fragment_source.c_str();

    glShaderSource(
        fragment_shader,
        1,
        &fragment_source_ptr,
        nullptr
    );

    glCompileShader(fragment_shader);

    /**
     * Проверяем результат компиляции fragment shader.
     */
    GLint fragment_success = 0;

    glGetShaderiv(
        fragment_shader,
        GL_COMPILE_STATUS,
        &fragment_success
    );

    if (fragment_success == GL_FALSE) {
        std::string log = GetShaderInfoLog(fragment_shader);

        /**
         * Vertex shader уже был успешно создан,
         * поэтому при ошибке fragment shader
         * освобождаются оба shader object.
         */
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        throw std::runtime_error(
            "Failed to compile fragment shader:\n" + log
        );
    }

    /**
     * Создаём итоговую OpenGL shader program.
     */
    program_id_ = glCreateProgram();

    /**
     * Подключаем успешно скомпилированные
     * vertex и fragment shader object.
     */
    glAttachShader(program_id_, vertex_shader);
    glAttachShader(program_id_, fragment_shader);

    /**
     * Связываем shader stages
     * в единую executable program.
     */
    glLinkProgram(program_id_);

    /**
     * Проверяем результат linking.
     */
    GLint link_success = 0;

    glGetProgramiv(
        program_id_,
        GL_LINK_STATUS,
        &link_success
    );

    if (link_success == GL_FALSE) {
        /**
         * Для shader program используется
         * program info log, а не shader info log.
         */
        std::string log = GetProgramInfoLog(program_id_);

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        glDeleteProgram(program_id_);
        program_id_ = 0;

        throw std::runtime_error(
            "Failed to link shader program:\n" + log
        );
    }

    /**
     * После успешного linking отдельные shader objects
     * больше не нужны.
     *
     * Связанная shader program продолжает существовать
     * независимо от них.
     */
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

/**
 * @brief Освобождает OpenGL shader program.
 *
 * Shader владеет program_id_,
 * поэтому OpenGL resource удаляется
 * при уничтожении C++ объекта.
 */
Shader::~Shader() {
    if (program_id_ != 0) {
        glDeleteProgram(program_id_);
        program_id_ = 0;
    }
}

/**
 * @brief Делает shader program текущей.
 *
 * После вызова следующие OpenGL draw calls
 * используют program_id_ до следующего glUseProgram().
 */
void Shader::Use() const {
    glUseProgram(program_id_);
}

/**
 * @brief Передаёт значение типа int в GLSL uniform.
 *
 * Сначала через glGetUniformLocation()
 * определяется location uniform-переменной,
 * после чего значение записывается через glUniform1i().
 *
 * @param name Имя uniform-переменной.
 * @param value Передаваемое значение.
 */
void Shader::SetInt(const std::string& name, int value) const {
    GLint location =
        glGetUniformLocation(
            program_id_,
            name.c_str()
        );

    glUniform1i(
        location,
        value
    );
}

/**
 * @brief Передаёт значение типа float в GLSL uniform.
 *
 * @param name Имя uniform-переменной.
 * @param value Передаваемое значение.
 */
void Shader::SetFloat(const std::string& name, float value) const {
    GLint location =
        glGetUniformLocation(
            program_id_,
            name.c_str()
        );

    glUniform1f(
        location,
        value
    );
}

/**
 * @brief Передаёт Vec4 в GLSL uniform.
 *
 * Компоненты x, y, z и w передаются
 * через glUniform4f().
 *
 * @param name Имя uniform-переменной.
 * @param value Передаваемый Vec4.
 */
void Shader::SetVec4(const std::string& name, const Vec4& value) const {
    const GLint location =
        glGetUniformLocation(
            program_id_,
            name.c_str()
        );

    glUniform4f(
        location,
        value.x,
        value.y,
        value.z,
        value.w
    );
}

/**
 * @brief Передаёт Matrix4 в GLSL mat4 uniform.
 *
 * Внутреннее представление Matrix4
 * использует доступ matrix[row][column].
 *
 * Перед передачей в OpenGL матрица преобразуется
 * в column-major массив через
 * AffineTransformation::GetColMajor().
 *
 * После этого данные передаются
 * через glUniformMatrix4fv().
 *
 * Параметр transpose устанавливается в GL_FALSE,
 * поскольку данные уже подготовлены
 * в требуемом порядке.
 *
 * @param name Имя mat4 uniform.
 * @param value Передаваемая Matrix4.
 */
void Shader::SetMat4(const std::string& name, const Matrix4& value) const {
    const GLint location =
        glGetUniformLocation(
            program_id_,
            name.c_str()
        );

    const std::array<float, 16> col_major =
        AffineTransformation::GetColMajor(
            value
        );

    glUniformMatrix4fv(
        location,
        1,
        GL_FALSE,
        col_major.data()
    );
}

/**
 * @brief Передаёт Vec3 в GLSL uniform.
 *
 * Location uniform-переменной определяется
 * через glGetUniformLocation(),
 * после чего компоненты x, y и z
 * передаются через glUniform3f().
 *
 * @param name Имя uniform-переменной.
 * @param value Передаваемый Vec3.
 */
void Shader::SetVec3(const std::string& name, const Vec3& value) {
    glUniform3f(
        glGetUniformLocation(
            program_id_,
            name.c_str()
        ),
        value.x,
        value.y,
        value.z
    );
}