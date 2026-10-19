#pragma once

#include <filesystem>

/**
 * @brief Управляет двумерной OpenGL texture.
 *
 * Texture2D загружает изображение с диска,
 * создаёт соответствующий GL_TEXTURE_2D
 * и управляет временем жизни OpenGL-ресурса.
 *
 * Класс хранит:
 *
 * - OpenGL texture ID;
 * - ширину изображения;
 * - высоту изображения;
 * - количество каналов.
 *
 * Texture2D реализует RAII:
 * OpenGL texture создаётся в конструкторе
 * и освобождается в деструкторе.
 *
 * Копирование запрещено,
 * поскольку несколько объектов Texture2D
 * не должны владеть одним и тем же OpenGL handle.
 *
 * Владение texture может передаваться
 * через move constructor и move assignment operator.
 */
class Texture2D {
public:
    /**
     * @brief Загружает изображение и создаёт OpenGL texture.
     *
     * Изображение загружается из указанного файла,
     * после чего его pixel data передаются в GPU.
     *
     * В текущей реализации поддерживаются изображения
     * с 1, 3 или 4 каналами.
     *
     * Для создания OpenGL texture
     * необходим активный OpenGL-контекст.
     *
     * @param path Путь к файлу изображения.
     *
     * @throws std::runtime_error Если изображение
     * не удалось загрузить или количество каналов
     * не поддерживается.
     */
    explicit Texture2D(const std::filesystem::path& path);

    /**
     * @brief Освобождает принадлежащую объекту OpenGL texture.
     *
     * Если texture_id_ не равен 0,
     * ресурс удаляется через glDeleteTextures().
     */
    ~Texture2D();

    /**
     * @brief Запрещает копирующее создание Texture2D.
     *
     * Два объекта не должны одновременно
     * владеть одним OpenGL texture ID.
     */
    Texture2D(const Texture2D&) = delete;

    /**
     * @brief Запрещает копирующее присваивание Texture2D.
     */
    Texture2D& operator=(const Texture2D&) = delete;

    /**
     * @brief Передаёт владение OpenGL texture
     * от другого объекта Texture2D.
     *
     * Вместе с texture ID передаются
     * размеры изображения и количество каналов.
     *
     * @param other Texture2D,
     * ресурс которого передаётся текущему объекту.
     */
    Texture2D(Texture2D&& other) noexcept;

    /**
     * @brief Перемещающий оператор присваивания.
     *
     * Текущий OpenGL-ресурс освобождается,
     * после чего объект принимает texture ID
     * и параметры изображения от other.
     *
     * @param other Texture2D,
     * ресурс которого необходимо получить.
     *
     * @return Ссылка на текущий Texture2D.
     */
    Texture2D& operator=(Texture2D&& other) noexcept;

    /**
     * @brief Привязывает texture к указанному texture unit.
     *
     * Сначала активируется texture unit:
     *
     * GL_TEXTURE0 + slot
     *
     * после чего texture_id_ привязывается
     * как GL_TEXTURE_2D.
     *
     * Например:
     *
     * - slot = 0 -> GL_TEXTURE0;
     * - slot = 1 -> GL_TEXTURE1.
     *
     * @param slot Номер texture unit.
     */
    void Bind(unsigned int slot = 0) const;

    /**
     * @brief Возвращает OpenGL ID texture.
     *
     * @return OpenGL texture handle.
     */
    unsigned int GetId() const;

    /**
     * @brief Возвращает ширину загруженного изображения.
     *
     * @return Ширина texture в пикселях.
     */
    int GetWidth() const;

    /**
     * @brief Возвращает высоту загруженного изображения.
     *
     * @return Высота texture в пикселях.
     */
    int GetHeight() const;

    /**
     * @brief Возвращает количество каналов изображения.
     *
     * В текущей реализации поддерживаются:
     *
     * - 1 канал;
     * - 3 канала;
     * - 4 канала.
     *
     * @return Количество каналов исходного изображения.
     */
    int GetChannels() const;

private:
    /**
     * @brief OpenGL ID двумерной texture.
     *
     * Значение 0 означает отсутствие
     * принадлежащего объекту OpenGL-ресурса.
     */
    unsigned int texture_id_ = 0;

    /**
     * @brief Ширина изображения в пикселях.
     */
    int width_ = 0;

    /**
     * @brief Высота изображения в пикселях.
     */
    int height_ = 0;

    /**
     * @brief Количество каналов исходного изображения.
     */
    int channels_ = 0;
};