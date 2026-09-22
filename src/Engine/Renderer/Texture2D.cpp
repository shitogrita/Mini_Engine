#include "Engine/Renderer/Texture2D.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Engine/ThirdParty/stb/stb_image.h"

#include <stdexcept>
#include <utility>


Texture2D::Texture2D( const std::filesystem::path& path ) {
    /*
     * OpenGL считает начало texture
     * снизу слева, а большинство изображений —
     * сверху слева.
     *
     * Поэтому переворачиваем изображение
     * вертикально во время загрузки.
     */
    stbi_set_flip_vertically_on_load(true);


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
 * сохраняем его диагностическое сообщение.
 *
 * Это важно для различения случаев:
 *
 * - файл отсутствует;
 * - JPEG имеет неподдерживаемый формат;
 * - файл повреждён;
 * - расширение файла не соответствует содержимому.
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

    GLenum format = GL_RGB;


    if (channels_ == 1) {
        format = GL_RED;
    }
    else if (channels_ == 3) {
        format = GL_RGB;
    }
    else if (channels_ == 4) {
        format = GL_RGBA;
    }
    else {
        stbi_image_free(data);

        throw std::runtime_error(
            "Unsupported texture channel count: " +
            std::to_string(
                channels_
            )
        );
    }


    glGenTextures(
        1,
        &texture_id_
    );


    glBindTexture(
        GL_TEXTURE_2D,
        texture_id_
    );


    /*
     * Поведение texture coordinates
     * за пределами диапазона [0, 1].
     *
     * GL_REPEAT означает повторение texture.
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


    /*
     * Фильтрация при уменьшении texture.
     *
     * Используем mipmaps.
     */
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );


    /*
     * Фильтрация при увеличении texture.
     */
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );


    /*
     * Передаём пиксели изображения
     * из RAM в GPU.
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


    /*
     * Создаём уменьшенные версии texture.
     *
     * Они используются, когда объект находится
     * далеко от Camera.
     */
    glGenerateMipmap(
        GL_TEXTURE_2D
    );


    /*
     * После glTexImage2D данные изображения
     * уже находятся в GPU.
     *
     * CPU-копия больше не нужна.
     */
    stbi_image_free(
        data
    );


    glBindTexture(
        GL_TEXTURE_2D,
        0
    );
}


Texture2D::~Texture2D()
{
    if (texture_id_ != 0) {
        glDeleteTextures(
            1,
            &texture_id_
        );
    }
}


Texture2D::Texture2D(
    Texture2D&& other
) noexcept
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
    )
{
}


Texture2D& Texture2D::operator=(
    Texture2D&& other
) noexcept
{
    if (this == &other) {
        return *this;
    }


    if (texture_id_ != 0) {
        glDeleteTextures(
            1,
            &texture_id_
        );
    }


    texture_id_ =
        std::exchange(
            other.texture_id_,
            0
        );

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


void Texture2D::Bind(
    unsigned int slot
) const
{
    glActiveTexture(
        GL_TEXTURE0 +
        slot
    );

    glBindTexture(
        GL_TEXTURE_2D,
        texture_id_
    );
}


unsigned int Texture2D::GetId() const
{
    return texture_id_;
}


int Texture2D::GetWidth() const
{
    return width_;
}


int Texture2D::GetHeight() const
{
    return height_;
}


int Texture2D::GetChannels() const
{
    return channels_;
}