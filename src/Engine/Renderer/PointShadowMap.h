#pragma once

/**
 * @brief Depth Cubemap для теней от Point Light.
 *
 * Точечный источник света светит во все стороны,
 * поэтому обычной двумерной Shadow Map недостаточно.
 *
 * PointShadowMap создаёт Cubemap из шести depth-texture:
 *
 * +X
 * -X
 * +Y
 * -Y
 * +Z
 * -Z
 *
 * Каждая грань хранит глубину сцены,
 * наблюдаемой из позиции Point Light.
 *
 * Класс отвечает только за OpenGL-ресурсы Shadow Map.
 * Он не знает ничего о SceneObject, PointLight или SceneViewport.
 */
class PointShadowMap {
public:
    /**
     * @brief Создаёт пустой PointShadowMap.
     *
     * OpenGL-ресурсы создаются отдельно через Initialize(),
     * когда OpenGL Context уже активен.
     */
    PointShadowMap() = default;

    /**
     * @brief Освобождает framebuffer и depth cubemap.
     *
     * Объект должен уничтожаться при активном OpenGL Context.
     */
    ~PointShadowMap();

    PointShadowMap(const PointShadowMap&) = delete;
    PointShadowMap& operator=(const PointShadowMap&) = delete;

    /**
     * @brief Создаёт framebuffer и Depth Cubemap.
     *
     * Повторный вызов после успешной инициализации
     * ничего не делает.
     *
     * @param resolution Размер одной стороны cubemap.
     * @return true, если framebuffer создан корректно.
     */
    bool Initialize(int resolution = 1024);

    /**
     * @brief Подготавливает одну грань cubemap к depth rendering.
     *
     * Метод:
     * - привязывает shadow framebuffer;
     * - подключает нужную грань cubemap;
     * - устанавливает shadow viewport;
     * - очищает depth buffer.
     *
     * @param face_index Индекс грани в диапазоне [0, 5].
     */
    void BeginFace(int face_index);

    /**
     * @brief Привязывает Depth Cubemap к texture unit.
     *
     * Например unit = 1 соответствует GL_TEXTURE1.
     *
     * @param unit Номер texture unit.
     */
    void BindTexture(unsigned int unit) const;

    /**
     * @brief Возвращает OpenGL ID Depth Cubemap.
     */
    unsigned int GetTextureId() const;

    /**
     * @brief Возвращает разрешение одной грани Shadow Map.
     */
    int GetResolution() const;

    /**
     * @brief Проверяет, создана ли Shadow Map.
     */
    bool IsInitialized() const;

private:
    /**
     * @brief Framebuffer, в который рендерятся шесть граней Cubemap.
     */
    unsigned int framebuffer_id_ = 0;

    /**
     * @brief Depth Cubemap.
     */
    unsigned int depth_cubemap_id_ = 0;

    /**
     * @brief Размер одной стороны Cubemap.
     */
    int resolution_ = 0;
};