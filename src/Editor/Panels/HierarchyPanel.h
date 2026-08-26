#pragma once

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneObject.h"

#include <QWidget>

#include <functional>
#include <memory>


class QTreeWidget;
class QTreeWidgetItem;


class HierarchyPanel : public QWidget {
public:
	explicit HierarchyPanel(
		QWidget* parent = nullptr
	);

	void SetScene(
		Scene* scene
	);

	void Refresh();

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
	QTreeWidget* tree_ = nullptr;

	Scene* scene_ = nullptr;

	std::function<void(
		std::shared_ptr<SceneObject>
	)> selection_changed_callback_;
};