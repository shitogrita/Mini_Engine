#include "Scene.h"

#include <algorithm>
#include <utility>

/**
 * @brief Добавляет SceneObject в сцену.
 *
 * Переданный std::shared_ptr перемещается
 * во внутренний контейнер Scene::objects_.
 *
 * После добавления объект становится частью сцены
 * и может использоваться другими системами,
 * работающими с результатом GetObjects().
 *
 * @param object Объект сцены, который необходимо добавить.
 */
void Scene::AddObject(std::shared_ptr<SceneObject> object) {
	objects_.push_back(
		std::move(object)
	);
}

/**
 * @brief Удаляет указанный SceneObject из сцены.
 *
 * Метод ищет переданный std::shared_ptr
 * во внутреннем контейнере objects_.
 *
 * Если object равен nullptr
 * или объект отсутствует в сцене,
 * удаление не выполняется.
 *
 * При успешном поиске соответствующий элемент
 * удаляется из objects_.
 *
 * @param object Объект, который необходимо удалить.
 *
 * @return true, если объект был найден и удалён.
 * @return false, если object равен nullptr
 * или объект отсутствует в сцене.
 */
bool Scene::RemoveObject(const std::shared_ptr<SceneObject>& object) {
	if (!object) {
		return false;
	}

	const auto iterator = std::find(
		objects_.begin(),
		objects_.end(),
		object
	);

	if (iterator == objects_.end()) {
		return false;
	}

	objects_.erase(iterator);

	return true;
}

/**
 * @brief Удаляет все объекты из сцены.
 *
 * Метод очищает внутренний контейнер objects_.
 *
 * Освобождение самих SceneObject происходит
 * в соответствии с правилами владения std::shared_ptr.
 */
void Scene::Clear() {
	objects_.clear();
}

/**
 * @brief Возвращает все объекты текущей сцены.
 *
 * Возвращается константная ссылка
 * на внутренний контейнер objects_,
 * поэтому через этот метод нельзя напрямую
 * изменять состав сцены.
 *
 * @return Константная ссылка на список SceneObject.
 */
const std::vector<std::shared_ptr<SceneObject>>& Scene::GetObjects() const {
	return objects_;
}