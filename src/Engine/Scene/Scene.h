#pragma once

#include "Engine/Scene/SceneObject.h"

#include <memory>
#include <vector>

/**
 * @brief Хранит набор объектов текущей сцены.
 *
 * Scene представляет контейнер верхнего уровня
 * для объектов типа SceneObject.
 *
 * Класс отвечает только за управление составом сцены:
 *
 * - добавление объектов;
 * - удаление объектов;
 * - очистку сцены;
 * - предоставление доступа к текущему набору объектов.
 *
 * Объекты хранятся через std::shared_ptr,
 * поэтому время их жизни может разделяться
 * между Scene и другими системами движка.
 */
class Scene {
public:
    /**
     * @brief Добавляет SceneObject в сцену.
     *
     * Переданный std::shared_ptr сохраняется
     * во внутреннем контейнере objects_.
     *
     * @param object Объект, который необходимо добавить.
     */
    void AddObject(std::shared_ptr<SceneObject> object);

    /**
     * @brief Удаляет указанный SceneObject из сцены.
     *
     * Если переданный указатель равен nullptr
     * или объект отсутствует в objects_,
     * метод возвращает false.
     *
     * @param object Объект, который необходимо удалить.
     *
     * @return true, если объект был найден и удалён.
     * @return false, если объект отсутствует
     * или передан nullptr.
     */
    bool RemoveObject(const std::shared_ptr<SceneObject>& object);

    /**
     * @brief Удаляет все объекты из сцены.
     *
     * Метод очищает внутренний контейнер objects_.
     */
    void Clear();

    /**
     * @brief Возвращает все объекты текущей сцены.
     *
     * Возвращается константная ссылка
     * на внутренний контейнер,
     * поэтому его состав нельзя напрямую изменить
     * через данный метод.
     *
     * @return Константная ссылка на список SceneObject.
     */
    const std::vector<std::shared_ptr<SceneObject>>& GetObjects() const;

private:
    /**
     * @brief Контейнер объектов сцены.
     *
     * Каждый элемент хранится через std::shared_ptr<SceneObject>,
     * что позволяет совместно владеть объектами
     * между Scene и другими подсистемами движка.
     */
    std::vector<std::shared_ptr<SceneObject>> objects_;
};