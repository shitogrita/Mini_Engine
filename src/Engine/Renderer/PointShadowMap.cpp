#include "Engine/Renderer/PointShadowMap.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"

#include <algorithm>

/**
 * @brief Освобождает OpenGL-ресурсы Point Shadow Map.
 */
PointShadowMap::~PointShadowMap() {
    if (depth_cubemap_id_ != 0) {
        glDeleteTextures(1, &depth_cubemap_id_);
        depth_cubemap_id_ = 0;
    }

    if (framebuffer_id_ != 0) {
        glDeleteFramebuffers(1, &framebuffer_id_);
        framebuffer_id_ = 0;
    }
}

/**
 * @brief Создаёт framebuffer и шесть depth-texture Cubemap.
 *
 * @param resolution Размер одной стороны Cubemap.
 * @return true, если framebuffer полностью корректен.
 */
bool PointShadowMap::Initialize(int resolution) {
    if (IsInitialized()) {
        return true;
    }

    resolution_ =
        std::max(
            resolution,
            1
        );

    /*
     * Создаём Depth Cubemap.
     */
    glGenTextures(
        1,
        &depth_cubemap_id_
    );

    glBindTexture(
        GL_TEXTURE_CUBE_MAP,
        depth_cubemap_id_
    );

    /*
     * Point Light смотрит во все стороны,
     * поэтому создаём шесть отдельных depth faces.
     */
    for (int face_index = 0; face_index < 6; ++face_index) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index,
            0,
            GL_DEPTH_COMPONENT,
            resolution_,
            resolution_,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr
        );
    }

    /*
     * Для shadow map линейная фильтрация пока не нужна.
     *
     * Мягкость тени позже получим PCF-выборками
     * непосредственно в Fragment Shader.
     */
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_MIN_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST
    );

    /*
     * Clamp to edge особенно важен на границах
     * между сторонами Cubemap.
     */
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_WRAP_R,
        GL_CLAMP_TO_EDGE
    );

    /*
     * Создаём framebuffer.
     *
     * Конкретная грань Cubemap будет подключаться
     * в BeginFace().
     */
    glGenFramebuffers(
        1,
        &framebuffer_id_
    );

    glBindFramebuffer(
        GL_FRAMEBUFFER,
        framebuffer_id_
    );

    /*
     * Shadow framebuffer не содержит Color Buffer.
     * Нас интересует только глубина.
     */
    glDrawBuffer(
        GL_NONE
    );

    glReadBuffer(
        GL_NONE
    );

    /*
     * Для проверки framebuffer временно подключаем
     * первую сторону Cubemap.
     */
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        depth_cubemap_id_,
        0
    );

    const bool framebuffer_complete =
        glCheckFramebufferStatus(
            GL_FRAMEBUFFER
        ) ==
        GL_FRAMEBUFFER_COMPLETE;

    /*
     * Здесь специально НЕ предполагаем,
     * что framebuffer 0 является framebuffer SceneViewport.
     *
     * QOpenGLWidget использует собственный framebuffer,
     * поэтому правильный framebuffer будет восстановлен
     * позже самим SceneViewport.
     */
    glBindFramebuffer(
        GL_FRAMEBUFFER,
        0
    );

    glBindTexture(
        GL_TEXTURE_CUBE_MAP,
        0
    );

    if (!framebuffer_complete) {
        glDeleteTextures(
            1,
            &depth_cubemap_id_
        );

        glDeleteFramebuffers(
            1,
            &framebuffer_id_
        );

        depth_cubemap_id_ = 0;
        framebuffer_id_ = 0;
        resolution_ = 0;

        return false;
    }

    return true;
}

/**
 * @brief Подготавливает одну грань Cubemap для Shadow Pass.
 *
 * @param face_index Индекс [0, 5].
 */
void PointShadowMap::BeginFace(int face_index) {
    if (!IsInitialized()) {
        return;
    }

    if (face_index < 0 || face_index >= 6) {
        return;
    }

    glBindFramebuffer(
        GL_FRAMEBUFFER,
        framebuffer_id_
    );

    /*
     * Для каждого направления Point Light
     * используем отдельную грань Depth Cubemap.
     */
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index,
        depth_cubemap_id_,
        0
    );

    glViewport(
        0,
        0,
        resolution_,
        resolution_
    );

    glClear(
        GL_DEPTH_BUFFER_BIT
    );
}

/**
 * @brief Привязывает Depth Cubemap к указанному Texture Unit.
 *
 * @param unit Номер texture unit.
 */
void PointShadowMap::BindTexture(unsigned int unit) const {
    if (!IsInitialized()) {
        return;
    }

    glActiveTexture(
        GL_TEXTURE0 +
        unit
    );

    glBindTexture(
        GL_TEXTURE_CUBE_MAP,
        depth_cubemap_id_
    );
}

/**
 * @brief Возвращает ID Depth Cubemap.
 */
unsigned int PointShadowMap::GetTextureId() const {
    return depth_cubemap_id_;
}

/**
 * @brief Возвращает разрешение Cubemap.
 */
int PointShadowMap::GetResolution() const {
    return resolution_;
}

/**
 * @brief Проверяет состояние OpenGL-ресурсов.
 */
bool PointShadowMap::IsInitialized() const {
    return framebuffer_id_ != 0 &&
           depth_cubemap_id_ != 0;
}