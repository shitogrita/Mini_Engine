#pragma once

#include <QWidget>

class QTreeWidget;

class HierarchyPanel final : public QWidget {
public:
	explicit HierarchyPanel(QWidget* parent = nullptr);

private:
	void CreateLayout();
	void FillPlaceholderScene();

	QTreeWidget* tree_ = nullptr;
};