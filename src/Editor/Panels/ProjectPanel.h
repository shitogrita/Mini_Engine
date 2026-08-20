#pragma once

#include <QWidget>

class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

class ProjectPanel final : public QWidget {
public:
	explicit ProjectPanel(QWidget* parent = nullptr);

	void AddImportedFile(const QString& file_path);

private:
	void CreateLayout();
	void FillPlaceholderFolders();

	QTreeWidgetItem* FindOrCreateModelsFolder();

	QLineEdit* search_field_ = nullptr;
	QTreeWidget* tree_ = nullptr;

	QTreeWidgetItem* assets_item_ = nullptr;
	QTreeWidgetItem* models_item_ = nullptr;
};