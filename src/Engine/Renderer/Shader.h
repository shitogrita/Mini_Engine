#pragma once

#include "Engine/Math/matrix_types.h"

#include <filesystem>
#include <string>

/**
 * @brief Управляет скомпилированной и слинкованной OpenGL shader program.
 *
 * Shader отвечает за:
 *
 * - загрузку GLSL-кода из файлов;
 * - компиляцию vertex shader;
 * - компиляцию fragment shader;
 * - linking shader stages в одну OpenGL program;
 * - активацию shader program;
 * - передачу uniform-переменных в GLSL.
 *
 * Класс владеет OpenGL program через program_id_.
 *
 * Копирование запрещено, поскольку два объекта Shader
 * не должны одновременно владеть одним и тем же
 * OpenGL resource.
 *
 * Владение shader program может передаваться
 * через move constructor и move assignment operator.
 */
class Shader {
public:
    /**
     * @brief Создаёт OpenGL shader program
     * из vertex и fragment shader файлов.
     *
     * Конструктор загружает GLSL source code,
     * компилирует оба shader stage
     * и выполняет linking итоговой program.
     *
     * Для выполнения необходим активный OpenGL-контекст.
     *
     * @param vertex_path Путь к vertex shader.
     * @param fragment_path Путь к fragment shader.
     *
     * @throws std::runtime_error Если файл не удалось открыть,
     * shader не скомпилировался
     * или shader program не удалось связать.
     */
    Shader(const std::filesystem::path& vertex_path, const std::filesystem::path& fragment_path);

    /**
     * @brief Освобождает OpenGL shader program.
     *
     * Если program_id_ не равен 0,
     * программа удаляется через glDeleteProgram().
     */
    ~Shader();

    /**
     * @brief Запрещает копирующее создание Shader.
     *
     * Два объекта не должны владеть
     * одним и тем же OpenGL program_id_.
     */
    Shader(const Shader&) = delete;

    /**
     * @brief Запрещает копирующее присваивание Shader.
     */
    Shader& operator=(const Shader&) = delete;

    /**
     * @brief Передаёт владение OpenGL program
     * от другого объекта Shader.
     *
     * @param other Shader,
     * ресурсы которого передаются текущему объекту.
     */
    Shader(Shader&& other) noexcept;

    /**
     * @brief Перемещающий оператор присваивания.
     *
     * Передаёт владение OpenGL program
     * от other текущему объекту.
     *
     * @param other Shader,
     * ресурсы которого необходимо получить.
     *
     * @return Ссылка на текущий Shader.
     */
    Shader& operator=(Shader&& other) noexcept;

    /**
     * @brief Делает shader program текущей.
     *
     * После вызова последующие OpenGL draw calls
     * используют program_id_.
     */
    void Use() const;

    /**
     * @brief Передаёт значение типа int в GLSL uniform.
     *
     * @param name Имя uniform-переменной.
     * @param value Передаваемое значение.
     */
    void SetInt(const std::string& name, int value) const;

    /**
     * @brief Передаёт значение типа float в GLSL uniform.
     *
     * @param name Имя uniform-переменной.
     * @param value Передаваемое значение.
     */
    void SetFloat(const std::string& name, float value) const;

    /**
     * @brief Передаёт Vec4 в GLSL uniform.
     *
     * @param name Имя uniform-переменной.
     * @param value Передаваемый Vec4.
     */
    void SetVec4(const std::string& name, const Vec4& value) const;

    /**
     * @brief Передаёт Matrix4 в GLSL mat4 uniform.
     *
     * Перед передачей матрица преобразуется
     * из внутреннего представления matrix[row][column]
     * в column-major массив.
     *
     * @param name Имя mat4 uniform-переменной.
     * @param value Передаваемая Matrix4.
     */
    void SetMat4(const std::string& name, const Matrix4& value) const;

    /**
     * @brief Передаёт Vec3 в GLSL uniform.
     *
     * @param name Имя uniform-переменной.
     * @param value Передаваемый Vec3.
     */
    void SetVec3(const std::string& name, const Vec3& value);

private:
    /**
     * @brief OpenGL ID слинкованной shader program.
     *
     * Значение 0 означает отсутствие
     * принадлежащей объекту OpenGL program.
     */
    unsigned int program_id_ = 0;
};