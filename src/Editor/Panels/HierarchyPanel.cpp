#include "Editor/Panels/HierarchyPanel.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

HierarchyPanel::HierarchyPanel(QWidget* parent)
	: QWidget(parent)
{
	CreateLayout();
	FillPlaceholderScene();
}

void HierarchyPanel::CreateLayout()
{
	QVBoxLayout* layout = new QVBoxLayout(this);

	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	tree_ = new QTreeWidget(this);

	tree_->setHeaderHidden(true);
	tree_->setAlternatingRowColors(false);
	tree_->setIndentation(18);

	layout->addWidget(tree_);
}

void HierarchyPanel::FillPlaceholderScene()
{
	QTreeWidgetItem* scene_item =
		new QTreeWidgetItem(QStringList{"Sample Scene"});

	QTreeWidgetItem* camera_item =
		new QTreeWidgetItem(QStringList{"Main Camera"});

	QTreeWidgetItem* light_item =
		new QTreeWidgetItem(QStringList{"Directional Light"});

	QTreeWidgetItem* cube_item =
		new QTreeWidgetItem(QStringList{"Cube"});

	scene_item->addChild(camera_item);
	scene_item->addChild(light_item);
	scene_item->addChild(cube_item);

	tree_->addTopLevelItem(scene_item);

	scene_item->setExpanded(true);
}