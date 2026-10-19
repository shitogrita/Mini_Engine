#pragma once

/**
 * @brief Управляет Depth Cubemap для теней от Point Light.
 *
 * Точечный источник света излучает свет во всех направлениях,
 * поэтому одной двумерной shadow map недостаточно.
 *
 * PointShadowMap использует одну Cubemap texture,
 * состоящую из шести depth faces:
 *
 * - +X;
 * - -X;
 * - +Y;
 * - -Y;
 * - +Z;
 * - -Z.
 *
 * Каждая сторона хранит информацию о глубине сцены,
 * наблюдаемой из позиции точечного источника света
 * в соответствующем направлении.
 *
 * Класс отвечает только за OpenGL-ресурсы shadow map:
 *
 * - framebuffer;
 * - Depth Cubemap texture;
 * - разрешение Cubemap.
 *
 * PointShadowMap не хранит информацию
 * о SceneObject, PointLight или SceneViewport.
 */
class PointShadowMap {
public:
    /**
     * @brief Создаёт неинициализированный PointShadowMap.
     *
     * OpenGL-ресурсы не создаются в конструкторе.
     *
     * Они создаются позже через Initialize(),
     * когда OpenGL-контекст уже активен.
     */
    PointShadowMap() = default;

    /**
     * @brief Освобождает OpenGL-ресурсы shadow map.
     *
     * Деструктор удаляет:
     *
     * - Depth Cubemap texture;
     * - framebuffer.
     *
     * Для корректного удаления OpenGL-ресурсов
     * должен существовать активный OpenGL-контекст.
     */
    ~PointShadowMap();

    /**
     * @brief Запрещает копирующее создание PointShadowMap.
     *
     * Объект владеет OpenGL handles,
     * поэтому несколько экземпляров не должны
     * владеть одними и теми же ресурсами.
     */
    PointShadowMap(const PointShadowMap&) = delete;

    /**
     * @brief Запрещает копирующее присваивание PointShadowMap.
     */
    PointShadowMap& operator=(const PointShadowMap&) = delete;

    /**
     * @brief Создаёт framebuffer и Depth Cubemap.
     *
     * Создаётся одна Cubemap texture
     * с шестью depth faces одинакового размера.
     *
     * Также создаётся framebuffer,
     * используемый для рендеринга глубины
     * в выбранную сторону Cubemap.
     *
     * Повторный вызов после успешной инициализации
     * не создаёт ресурсы заново.
     *
     * Если передано значение resolution меньше 1,
     * фактическое разрешение устанавливается равным 1.
     *
     * @param resolution Размер одной стороны Cubemap в пикселях.
     *
     * @return true, если OpenGL-ресурсы успешно созданы
     * и framebuffer имеет корректный статус;
     * иначе false.
     */
    bool Initialize(int resolution = 1024);

    /**
     * @brief Подготавливает одну сторону Cubemap
     * для depth rendering.
     *
     * Метод:
     *
     * - привязывает shadow framebuffer;
     * - подключает выбранную face Cubemap
     *   как GL_DEPTH_ATTACHMENT;
     * - устанавливает viewport размером resolution_;
     * - очищает depth buffer.
     *
     * Если PointShadowMap не инициализирован
     * или face_index находится вне диапазона [0, 5],
     * метод завершает работу без дальнейших действий.
     *
     * @param face_index Индекс стороны Cubemap
     * в диапазоне [0, 5].
     */
    void BeginFace(int face_index);

    /**
     * @brief Привязывает Depth Cubemap
     * к указанному texture unit.
     *
     * Например:
     *
     * unit = 0 -> GL_TEXTURE0
     *
     * unit = 1 -> GL_TEXTURE1
     *
     * Если shadow map не инициализирована,
     * метод завершает работу без изменения texture binding.
     *
     * @param unit Номер texture unit.
     */
    void BindTexture(unsigned int unit) const;

    /**
     * @brief Возвращает OpenGL ID Depth Cubemap.
     *
     * @return ID Cubemap texture.
     * Если texture не создана, значение равно 0.
     */
    unsigned int GetTextureId() const;

    /**
     * @brief Возвращает разрешение одной стороны Cubemap.
     *
     * @return Размер одной face Cubemap в пикселях.
     */
    int GetResolution() const;

    /**
     * @brief Проверяет состояние OpenGL-ресурсов shadow map.
     *
     * PointShadowMap считается инициализированным,
     * если одновременно существуют:
     *
     * - framebuffer_id_;
     * - depth_cubemap_id_.
     *
     * @return true, если основные OpenGL-ресурсы созданы;
     * иначе false.
     */
    bool IsInitialized() const;

private:
    /**
     * @brief OpenGL framebuffer для shadow pass.
     *
     * В framebuffer поочерёдно подключаются
     * шесть faces одной Depth Cubemap texture.
     */
    unsigned int framebuffer_id_ = 0;

    /**
     * @brief OpenGL ID Depth Cubemap texture.
     *
     * Texture содержит шесть depth faces,
     * соответствующих направлениям ±X, ±Y и ±Z.
     */
    unsigned int depth_cubemap_id_ = 0;

    /**
     * @brief Размер одной стороны Cubemap в пикселях.
     *
     * До успешной инициализации значение равно 0.
     */
    int resolution_ = 0;
};