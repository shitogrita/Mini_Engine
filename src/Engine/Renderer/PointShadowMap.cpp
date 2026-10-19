#include "Engine/Renderer/PointShadowMap.h"

#include "Engine/Platform/OpenGL/Glad/glad.h"

#include <algorithm>

/**
 * @brief Освобождает OpenGL-ресурсы Point Shadow Map.
 *
 * Удаляются:
 *
 * - Depth Cubemap texture;
 * - framebuffer, используемый для shadow pass.
 *
 * После удаления соответствующие OpenGL handles
 * сбрасываются в 0.
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
 * @brief Создаёт OpenGL-ресурсы для point light shadow mapping.
 *
 * Point Light излучает свет во всех направлениях,
 * поэтому карта глубины представлена Cubemap texture
 * с шестью отдельными faces.
 *
 * Метод создаёт:
 *
 * - одну Depth Cubemap texture;
 * - шесть depth faces одинакового разрешения;
 * - framebuffer для рендеринга глубины.
 *
 * Framebuffer не содержит color attachment,
 * поскольку во время shadow pass сохраняется
 * только информация о глубине.
 *
 * Если объект уже инициализирован,
 * повторное создание ресурсов не выполняется.
 *
 * Значение resolution ограничивается снизу единицей.
 *
 * @param resolution Размер одной стороны Cubemap в пикселях.
 *
 * @return true, если framebuffer успешно создан
 * и имеет статус GL_FRAMEBUFFER_COMPLETE;
 * иначе false.
 */
bool PointShadowMap::Initialize(int resolution) {
    if (IsInitialized()) {
        return true;
    }

    /**
     * Разрешение каждой стороны Cubemap
     * должно быть не меньше одного пикселя.
     */
    resolution_ = std::max(
        resolution,
        1
    );

    /**
     * Создаём одну Cubemap texture,
     * содержащую шесть depth faces.
     */
    glGenTextures(
        1,
        &depth_cubemap_id_
    );

    glBindTexture(
        GL_TEXTURE_CUBE_MAP,
        depth_cubemap_id_
    );

    /**
     * Point Light должен хранить глубину
     * для всех шести пространственных направлений.
     *
     * Поэтому память выделяется отдельно
     * для каждой стороны Cubemap.
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

    /**
     * Для текущей shadow map используется
     * nearest filtering.
     *
     * Дополнительное сглаживание тени
     * может выполняться выборками
     * непосредственно в fragment shader.
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

    /**
     * Ограничиваем координаты границами каждой face.
     *
     * Это особенно важно при выборке
     * вблизи границ Cubemap.
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

    /**
     * Создаём framebuffer для shadow pass.
     *
     * Конкретная face Depth Cubemap
     * будет подключаться к GL_DEPTH_ATTACHMENT
     * непосредственно в BeginFace().
     */
    glGenFramebuffers(
        1,
        &framebuffer_id_
    );

    glBindFramebuffer(
        GL_FRAMEBUFFER,
        framebuffer_id_
    );

    /**
     * Shadow framebuffer не использует color buffer.
     *
     * Во время этого прохода требуется
     * только сохранение depth values.
     */
    glDrawBuffer(
        GL_NONE
    );

    glReadBuffer(
        GL_NONE
    );

    /**
     * Для проверки полноты framebuffer
     * временно подключаем первую face Cubemap
     * как depth attachment.
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

    /**
     * После проверки снимаем binding framebuffer.
     *
     * Важно, что framebuffer с ID 0
     * не обязательно является framebuffer,
     * используемым SceneViewport.
     *
     * QOpenGLWidget работает со своим framebuffer,
     * поэтому его необходимое состояние
     * восстанавливается внешним кодом SceneViewport.
     */
    glBindFramebuffer(
        GL_FRAMEBUFFER,
        0
    );

    glBindTexture(
        GL_TEXTURE_CUBE_MAP,
        0
    );

    /**
     * Если framebuffer оказался некорректным,
     * удаляем уже созданные OpenGL-ресурсы
     * и возвращаем объект в неинициализированное состояние.
     */
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
 * @brief Подготавливает одну сторону Depth Cubemap для shadow pass.
 *
 * Метод:
 *
 * - привязывает shadow framebuffer;
 * - подключает выбранную Cubemap face к depth attachment;
 * - устанавливает viewport в разрешение shadow map;
 * - очищает depth buffer перед новым проходом.
 *
 * Индексы faces находятся в диапазоне [0, 5]
 * и соответствуют последовательным значениям:
 *
 * GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index.
 *
 * Если объект не инициализирован
 * или индекс находится вне допустимого диапазона,
 * метод завершает работу без изменения shadow map.
 *
 * @param face_index Индекс стороны Cubemap в диапазоне [0, 5].
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

    /**
     * Для каждого направления Point Light
     * используется отдельная face Depth Cubemap.
     */
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index,
        depth_cubemap_id_,
        0
    );

    /**
     * Shadow pass выполняется
     * в разрешении Depth Cubemap.
     */
    glViewport(
        0,
        0,
        resolution_,
        resolution_
    );

    /**
     * Перед рендерингом новой стороны
     * очищаем предыдущие depth values.
     */
    glClear(
        GL_DEPTH_BUFFER_BIT
    );
}

/**
 * @brief Привязывает Depth Cubemap к указанному texture unit.
 *
 * Метод используется перед чтением shadow map
 * из shader через samplerCube.
 *
 * Если PointShadowMap не инициализирован,
 * OpenGL state не изменяется.
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
 * @brief Возвращает OpenGL ID Depth Cubemap.
 *
 * @return ID Cubemap texture.
 * Если ресурс не создан, значение равно 0.
 */
unsigned int PointShadowMap::GetTextureId() const {
    return depth_cubemap_id_;
}

/**
 * @brief Возвращает разрешение одной стороны Depth Cubemap.
 *
 * @return Размер Cubemap face в пикселях.
 */
int PointShadowMap::GetResolution() const {
    return resolution_;
}

/**
 * @brief Проверяет наличие основных OpenGL-ресурсов shadow map.
 *
 * PointShadowMap считается инициализированным,
 * если одновременно существуют:
 *
 * - framebuffer;
 * - Depth Cubemap texture.
 *
 * @return true, если framebuffer_id_ и depth_cubemap_id_
 * не равны 0; иначе false.
 */
bool PointShadowMap::IsInitialized() const {
    return framebuffer_id_ != 0 &&
           depth_cubemap_id_ != 0;
}