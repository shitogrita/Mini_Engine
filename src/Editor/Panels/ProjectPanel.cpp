#include "Editor/Panels/ProjectPanel.h"

#include <QFileInfo>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

ProjectPanel::ProjectPanel(QWidget* parent) : QWidget(parent) {
    CreateLayout();
    CreateTree();

    connect(search_line_, &QLineEdit::textChanged, this, &ProjectPanel::FilterTree);
}

void ProjectPanel::CreateLayout() {
    auto* layout = new QVBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    search_line_ = new QLineEdit(this);
    search_line_->setPlaceholderText("Search assets...");
    search_line_->setClearButtonEnabled(true);

    tree_ = new QTreeWidget(this);
    tree_->setHeaderHidden(true);
    tree_->setIndentation(20);

    layout->addWidget(search_line_);
    layout->addWidget(tree_);
}

void ProjectPanel::CreateTree() {
    assets_item_ = new QTreeWidgetItem(tree_);
    assets_item_->setText(0, "Assets");

    models_item_ = new QTreeWidgetItem(assets_item_);
    models_item_->setText(0, "Models");

    textures_item_ = new QTreeWidgetItem(assets_item_);
    textures_item_->setText(0, "Textures");

    shaders_item_ = new QTreeWidgetItem(assets_item_);
    shaders_item_->setText(0, "Shaders");

    materials_item_ = new QTreeWidgetItem(assets_item_);
    materials_item_->setText(0, "Materials");

    assets_item_->setExpanded(true);
    models_item_->setExpanded(true);
}

void ProjectPanel::AddImportedFile(const QString& file_path) {
    if (!models_item_) {
        return;
    }

    const QFileInfo file_info(file_path);
    const QString absolute_path = file_info.absoluteFilePath();

    /*
     * Не добавляем один и тот же файл несколько раз
     * в Project Panel.
     */
    for (int i = 0; i < models_item_->childCount(); ++i) {
        QTreeWidgetItem* item = models_item_->child(i);

        if (item && item->data(0, Qt::UserRole).toString() == absolute_path) {
            tree_->setCurrentItem(item);
            return;
        }
    }

    auto* item = new QTreeWidgetItem(models_item_);

    item->setText(0, file_info.fileName());
    item->setData(0, Qt::UserRole, absolute_path);
    item->setToolTip(0, absolute_path);

    models_item_->setExpanded(true);
    tree_->setCurrentItem(item);
}

void ProjectPanel::ClearImportedFiles() {
    if (!models_item_) {
        return;
    }

    /*
     * takeChild() отсоединяет item от дерева,
     * delete уничтожает его.
     */
    while (models_item_->childCount() > 0) {
        delete models_item_->takeChild(0);
    }

    tree_->clearSelection();

    if (search_line_) {
        search_line_->clear();
    }

    models_item_->setExpanded(true);
}

void ProjectPanel::FilterTree(const QString& text) {
    if (!models_item_) {
        return;
    }

    const QString filter = text.trimmed();

    for (int i = 0; i < models_item_->childCount(); ++i) {
        QTreeWidgetItem* item = models_item_->child(i);

        if (!item) {
            continue;
        }

        const bool visible =
            filter.isEmpty() ||
            item->text(0).contains(filter, Qt::CaseInsensitive);

        item->setHidden(!visible);
    }
}