#pragma once

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneObject.h"

#include <QWidget>

#include <functional>
#include <memory>


class QTreeWidget;
class QTreeWidgetItem;


/**
 * @brief Панель иерархии объектов сцены.
 *
 * Отображает SceneObject, находящиеся в Scene,
 * и позволяет выбирать их через QTreeWidget.
 */
class HierarchyPanel : public QWidget {
public:

	explicit HierarchyPanel(
		QWidget* parent = nullptr
	);


	/**
	 * @brief Устанавливает Scene,
	 * содержимое которой отображается в Hierarchy.
	 */
	void SetScene(
		Scene* scene
	);


	/**
	 * @brief Полностью обновляет список объектов.
	 */
	void Refresh();


	/**
	 * @brief Программно выделяет SceneObject в Hierarchy.
	 *
	 * Используется, когда объект выбирается
	 * непосредственно во Viewport.
	 */
	void SetSelectedObject(
		const std::shared_ptr<SceneObject>& object
	);


	/**
	 * @brief Устанавливает callback выбора объекта
	 * пользователем внутри Hierarchy.
	 */
	void SetSelectionChangedCallback(
		std::function<void(
			std::shared_ptr<SceneObject>
		)> callback
	);


private:

	void CreateLayout();

	void FillPlaceholderScene();

	void HandleSelectionChanged();


private:

	QTreeWidget* tree_ =
		nullptr;


	Scene* scene_ =
		nullptr;


	std::function<void(
		std::shared_ptr<SceneObject>
	)>
		selection_changed_callback_;
};