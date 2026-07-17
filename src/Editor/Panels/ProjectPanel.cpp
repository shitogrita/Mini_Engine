#include "Editor/Panels/ProjectPanel.h"

#include <QFileInfo>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

ProjectPanel::ProjectPanel(QWidget* parent)
    : QWidget(parent)
{
    CreateLayout();
    FillPlaceholderFolders();
}

void ProjectPanel::CreateLayout()
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    search_field_ = new QLineEdit(this);
    search_field_->setPlaceholderText("Search assets...");

    tree_ = new QTreeWidget(this);

    tree_->setHeaderHidden(true);
    tree_->setIndentation(18);

    layout->addWidget(search_field_);
    layout->addWidget(tree_);

    connect(
        search_field_,
        &QLineEdit::textChanged,
        this,
        [this](const QString& text)
        {
            const QString normalized_text =
                text.trimmed();

            for (
                int index = 0;
                index < tree_->topLevelItemCount();
                ++index
            ) {
                QTreeWidgetItem* item =
                    tree_->topLevelItem(index);

                for (
                    int child_index = 0;
                    child_index < item->childCount();
                    ++child_index
                ) {
                    QTreeWidgetItem* child =
                        item->child(child_index);

                    const bool matches =
                        normalized_text.isEmpty() ||
                        child->text(0).contains(
                            normalized_text,
                            Qt::CaseInsensitive
                        );

                    child->setHidden(!matches);
                }
            }
        }
    );
}

void ProjectPanel::FillPlaceholderFolders()
{
    assets_item_ =
        new QTreeWidgetItem(QStringList{"Assets"});

    models_item_ =
        new QTreeWidgetItem(QStringList{"Models"});

    QTreeWidgetItem* textures_item =
        new QTreeWidgetItem(QStringList{"Textures"});

    QTreeWidgetItem* shaders_item =
        new QTreeWidgetItem(QStringList{"Shaders"});

    QTreeWidgetItem* materials_item =
        new QTreeWidgetItem(QStringList{"Materials"});

    assets_item_->addChild(models_item_);
    assets_item_->addChild(textures_item);
    assets_item_->addChild(shaders_item);
    assets_item_->addChild(materials_item);

    tree_->addTopLevelItem(assets_item_);

    assets_item_->setExpanded(true);
    models_item_->setExpanded(true);
}

void ProjectPanel::AddImportedFile(
    const QString& file_path
)
{
    QTreeWidgetItem* models_folder =
        FindOrCreateModelsFolder();

    const QFileInfo file_info(file_path);
    const QString file_name = file_info.fileName();

    for (
        int index = 0;
        index < models_folder->childCount();
        ++index
    ) {
        QTreeWidgetItem* child =
            models_folder->child(index);

        if (child->text(0) == file_name) {
            tree_->setCurrentItem(child);
            return;
        }
    }

    QTreeWidgetItem* file_item =
        new QTreeWidgetItem(QStringList{file_name});

    file_item->setToolTip(0, file_path);

    models_folder->addChild(file_item);
    models_folder->setExpanded(true);

    tree_->setCurrentItem(file_item);
}

QTreeWidgetItem* ProjectPanel::FindOrCreateModelsFolder()
{
    if (models_item_ != nullptr) {
        return models_item_;
    }

    models_item_ =
        new QTreeWidgetItem(QStringList{"Models"});

    assets_item_->addChild(models_item_);

    return models_item_;
}