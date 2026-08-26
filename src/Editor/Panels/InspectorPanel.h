#pragma once

#include "Engine/Scene/SceneObject.h"

#include <QWidget>

#include <functional>
#include <memory>


class QLabel;
class QDoubleSpinBox;


class InspectorPanel : public QWidget {
public:
	explicit InspectorPanel(
		QWidget* parent = nullptr
	);

	void SetObjectName(
		const QString& object_name
	);

	void SetSelectedObject(
		std::shared_ptr<SceneObject> object
	);

	void ClearSelection();

	void SetTransformChangedCallback(
		std::function<void()> callback
	);

private:
	void CreateLayout();

	void UpdateTransformFields();

	QDoubleSpinBox* CreateTransformSpinBox();

private:
	QLabel* title_label_ = nullptr;

	QLabel* transform_label_ = nullptr;

	QLabel* information_label_ = nullptr;


	QDoubleSpinBox* position_x_ = nullptr;
	QDoubleSpinBox* position_y_ = nullptr;
	QDoubleSpinBox* position_z_ = nullptr;

	QDoubleSpinBox* rotation_x_ = nullptr;
	QDoubleSpinBox* rotation_y_ = nullptr;
	QDoubleSpinBox* rotation_z_ = nullptr;

	QDoubleSpinBox* scale_x_ = nullptr;
	QDoubleSpinBox* scale_y_ = nullptr;
	QDoubleSpinBox* scale_z_ = nullptr;


	std::shared_ptr<SceneObject>
		selected_object_;

	std::function<void()>
		transform_changed_callback_;

	bool updating_fields_ = false;
};