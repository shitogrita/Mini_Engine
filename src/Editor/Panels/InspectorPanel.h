#pragma once

#include <QWidget>

class QLabel;

class InspectorPanel final : public QWidget {
public:
	explicit InspectorPanel(QWidget* parent = nullptr);

	void SetObjectName(const QString& object_name);
	void ClearSelection();

private:
	void CreateLayout();

	QLabel* title_label_ = nullptr;
	QLabel* transform_label_ = nullptr;
	QLabel* information_label_ = nullptr;
};