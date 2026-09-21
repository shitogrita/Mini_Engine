#pragma once

#include <QString>
#include <QWidget>

class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

class ProjectPanel final : public QWidget {
public:
	explicit ProjectPanel(QWidget* parent = nullptr);

	void AddImportedFile(const QString& file_path);
	void ClearImportedFiles();

private:
	void CreateLayout();
	void CreateTree();
	void FilterTree(const QString& text);

	QLineEdit* search_line_ = nullptr;
	QTreeWidget* tree_ = nullptr;

	QTreeWidgetItem* assets_item_ = nullptr;
	QTreeWidgetItem* models_item_ = nullptr;
	QTreeWidgetItem* textures_item_ = nullptr;
	QTreeWidgetItem* shaders_item_ = nullptr;
	QTreeWidgetItem* materials_item_ = nullptr;
};