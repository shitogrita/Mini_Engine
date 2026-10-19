#include "Engine/Renderer/Texture2D.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Engine/ThirdParty/stb/stb_image.h"

#include <stdexcept>
#include <utility>

/**
 * @brief Загружает изображение из файла и создаёт OpenGL 2D texture.
 *
 * Изображение декодируется библиотекой stb_image,
 * после чего полученные pixel data передаются в GPU.
 *
 * Последовательность создания texture:
 *
 * - включается вертикальное отражение изображения при загрузке;
 * - изображение декодируется через stbi_load();
 * - определяется OpenGL format по количеству каналов;
 * - создаётся OpenGL texture;
 * - настраиваются wrapping и filtering;
 * - pixel data передаются через glTexImage2D();
 * - генерируются mipmaps;
 * - CPU-копия изображения освобождается.
 *
 * Поддерживаются изображения с количеством каналов:
 *
 * - 1 — GL_RED;
 * - 3 — GL_RGB;
 * - 4 — GL_RGBA.
 *
 * Для выполнения OpenGL-операций необходим
 * активный OpenGL-контекст.
 *
 * @param path Путь к файлу изображения.
 *
 * @throws std::runtime_error Если stb_image
 * не смог загрузить изображение
 * или количество каналов не поддерживается.
 */
Texture2D::Texture2D(const std::filesystem::path& path) {
    /**
     * Переворачиваем изображение по вертикали
     * непосредственно во время загрузки через stb_image.
     *
     * Это приводит порядок строк изображения
     * в соответствие с используемой системой
     * текстурных координат.
     */
    stbi_set_flip_vertically_on_load(true);

    /**
     * Загружаем изображение в CPU memory.
     *
     * stb_image заполняет:
     *
     * - width_;
     * - height_;
     * - channels_.
     *
     * Последний аргумент 0 означает,
     * что количество каналов исходного изображения
     * не преобразуется принудительно.
     */
    unsigned char* data =
        stbi_load(
            path.string().c_str(),
            &width_,
            &height_,
            &channels_,
            0
        );

    /**
     * Если stb_image не смог декодировать изображение,
     * сохраняем диагностическое сообщение библиотеки.
     *
     * Это позволяет получить дополнительную информацию
     * о причине ошибки загрузки.
     */
    if (data == nullptr) {
        const char* failure_reason =
            stbi_failure_reason();

        throw std::runtime_error(
            "Failed to load texture: " +
            path.string() +
            " | stb_image: " +
            (
                failure_reason != nullptr
                    ? std::string(failure_reason)
                    : std::string("unknown error")
            )
        );
    }

    /**
     * Определяем OpenGL pixel format
     * по фактическому количеству каналов изображения.
     */
    GLenum format = GL_RGB;

    if (channels_ == 1) {
        format = GL_RED;
    } else if (channels_ == 3) {
        format = GL_RGB;
    } else if (channels_ == 4) {
        format = GL_RGBA;
    } else {
        /**
         * CPU memory необходимо освободить
         * перед завершением конструктора с исключением.
         */
        stbi_image_free(data);

        throw std::runtime_error(
            "Unsupported texture channel count: " +
            std::to_string(
                channels_
            )
        );
    }

    /**
     * Создаём OpenGL texture object.
     */
    glGenTextures(
        1,
        &texture_id_
    );

    glBindTexture(
        GL_TEXTURE_2D,
        texture_id_
    );

    /**
     * Определяем поведение texture coordinates
     * за пределами диапазона [0, 1].
     *
     * GL_REPEAT повторяет изображение
     * по соответствующей координатной оси.
     */
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    /**
     * При уменьшении texture используется
     * линейная фильтрация между mipmap levels.
     */
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    /**
     * При увеличении texture используется
     * обычная линейная фильтрация.
     */
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    /**
     * Передаём pixel data из CPU memory
     * в OpenGL texture.
     *
     * Внутренний формат и формат входных данных
     * определяются количеством каналов изображения.
     */
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(
            format
        ),
        width_,
        height_,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );

    /**
     * Генерируем mipmap chain
     * для созданной texture.
     */
    glGenerateMipmap(
        GL_TEXTURE_2D
    );

    /**
     * После glTexImage2D() pixel data
     * уже переданы в OpenGL.
     *
     * CPU-копия изображения больше не требуется.
     */
    stbi_image_free(
        data
    );

    /**
     * Снимаем binding созданной 2D texture.
     */
    glBindTexture(
        GL_TEXTURE_2D,
        0
    );
}

/**
 * @brief Освобождает принадлежащую объекту OpenGL texture.
 *
 * Если texture_id_ содержит корректный OpenGL handle,
 * ресурс удаляется через glDeleteTextures().
 */
Texture2D::~Texture2D() {
    if (texture_id_ != 0) {
        glDeleteTextures(
            1,
            &texture_id_
        );
    }
}

/**
 * @brief Передаёт владение OpenGL texture
 * от другого объекта Texture2D.
 *
 * Перемещаются:
 *
 * - texture_id_;
 * - width_;
 * - height_;
 * - channels_.
 *
 * std::exchange() одновременно получает прежнее значение
 * и сбрасывает соответствующее поле other в 0.
 *
 * Благодаря этому перемещённый объект
 * больше не владеет переданной OpenGL texture.
 *
 * @param other Texture2D,
 * ресурс которого передаётся текущему объекту.
 */
Texture2D::Texture2D(Texture2D&& other) noexcept
    : texture_id_(
          std::exchange(
              other.texture_id_,
              0
          )
      ),
      width_(
          std::exchange(
              other.width_,
              0
          )
      ),
      height_(
          std::exchange(
              other.height_,
              0
          )
      ),
      channels_(
          std::exchange(
              other.channels_,
              0
          )
      ) {
}

/**
 * @brief Перемещающий оператор присваивания.
 *
 * Перед получением нового ресурса
 * текущая OpenGL texture удаляется,
 * если она существует.
 *
 * После этого владение texture и её параметрами
 * передаётся от other текущему объекту.
 *
 * @param other Texture2D,
 * ресурс которого необходимо получить.
 *
 * @return Ссылка на текущий Texture2D.
 */
Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    /**
     * Освобождаем OpenGL texture,
     * которой текущий объект владел до присваивания.
     */
    if (texture_id_ != 0) {
        glDeleteTextures(
            1,
            &texture_id_
        );
    }

    /**
     * Передаём владение OpenGL handle.
     */
    texture_id_ =
        std::exchange(
            other.texture_id_,
            0
        );

    /**
     * Передаём характеристики изображения.
     */
    width_ =
        std::exchange(
            other.width_,
            0
        );

    height_ =
        std::exchange(
            other.height_,
            0
        );

    channels_ =
        std::exchange(
            other.channels_,
            0
        );

    return *this;
}

/**
 * @brief Привязывает texture к указанному texture unit.
 *
 * Сначала активируется:
 *
 * GL_TEXTURE0 + slot
 *
 * после чего texture_id_ привязывается
 * как GL_TEXTURE_2D.
 *
 * @param slot Номер texture unit.
 */
void Texture2D::Bind(unsigned int slot) const {
    glActiveTexture(
        GL_TEXTURE0 +
        slot
    );

    glBindTexture(
        GL_TEXTURE_2D,
        texture_id_
    );
}

/**
 * @brief Возвращает OpenGL ID texture.
 *
 * @return OpenGL texture handle.
 */
unsigned int Texture2D::GetId() const {
    return texture_id_;
}

/**
 * @brief Возвращает ширину загруженного изображения.
 *
 * @return Ширина texture в пикселях.
 */
int Texture2D::GetWidth() const {
    return width_;
}

/**
 * @brief Возвращает высоту загруженного изображения.
 *
 * @return Высота texture в пикселях.
 */
int Texture2D::GetHeight() const {
    return height_;
}

/**
 * @brief Возвращает количество каналов исходного изображения.
 *
 * Для поддерживаемых изображений значение может быть:
 *
 * - 1;
 * - 3;
 * - 4.
 *
 * @return Количество каналов изображения.
 */
int Texture2D::GetChannels() const {
    return channels_;
}