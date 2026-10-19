#include "Editor/Panels/HierarchyPanel.h"

#include <QAbstractItemView>
#include <QFont>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <cstddef>
#include <utility>

/**
 * @brief Возвращает человекочитаемое имя типа SceneObject для tooltip.
 */
static QString GetSceneObjectTypeName(SceneObject::Type type) {
    switch (type) {
        case SceneObject::Type::Cube:
            return "Cube";
        case SceneObject::Type::Plane:
            return "Plane";
        case SceneObject::Type::Sphere:
            return "Sphere";
        case SceneObject::Type::ImportedModel:
            return "Imported Model";
        case SceneObject::Type::PointLight:
            return "Point Light";
        case SceneObject::Type::Empty:
            return "Empty";
    }

    return "Scene Object";
}

/**
 * @brief Создаёт Hierarchy и подготавливает визуальную структуру панели.
 *
 * До передачи реальной Scene отображается только пустой корневой узел.
 *
 * @param parent Родительский QWidget.
 */
HierarchyPanel::HierarchyPanel(QWidget* parent) : QWidget(parent) {
    CreateLayout();
    FillPlaceholderScene();
}

/**
 * @brief Создаёт QTreeWidget и применяет компактное оформление в серых тонах.
 *
 * Визуальный стиль намеренно нейтральный: активный объект выделяется
 * светло-серой строкой без яркого цветового акцента, чтобы основное внимание
 * оставалось на Scene View и Inspector.
 */
void HierarchyPanel::CreateLayout() {
    setObjectName("HierarchyPanel");

    setStyleSheet(
        R"(
            QWidget#HierarchyPanel {
                background-color: #232529;
            }

            QTreeWidget {
                background-color: #232529;
                color: #cfd2d6;
                border: none;
                outline: none;
                padding: 6px 4px;
                show-decoration-selected: 1;
            }

            QTreeWidget::item {
                min-height: 26px;
                padding: 2px 5px;
                border-radius: 4px;
            }

            QTreeWidget::item:hover {
                background-color: #2d3035;
            }

            QTreeWidget::item:selected {
                background-color: #44484e;
                color: #f1f2f3;
            }

            QTreeWidget::branch {
                background-color: transparent;
            }

            QScrollBar:vertical {
                background-color: #232529;
                width: 10px;
                margin: 0px;
            }

            QScrollBar::handle:vertical {
                background-color: #4a4e54;
                border-radius: 5px;
                min-height: 28px;
            }

            QScrollBar::handle:vertical:hover {
                background-color: #5a5f66;
            }

            QScrollBar::add-line:vertical,
            QScrollBar::sub-line:vertical {
                height: 0px;
            }
        )"
    );

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tree_ = new QTreeWidget(this);
    tree_->setHeaderHidden(true);
    tree_->setAlternatingRowColors(false);
    tree_->setIndentation(16);
    tree_->setAnimated(false);
    tree_->setUniformRowHeights(true);
    tree_->setRootIsDecorated(true);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addWidget(tree_);

    connect(tree_, &QTreeWidget::itemSelectionChanged, this, [this]() {
        HandleSelectionChanged();
    });
}

/**
 * @brief Создаёт нейтральное пустое состояние Hierarchy.
 *
 * В отличие от демонстрационных placeholder-объектов здесь не создаются
 * фиктивные Camera, Light или Cube: до привязки Scene панель показывает
 * только корень дерева.
 */
void HierarchyPanel::FillPlaceholderScene() {
    tree_->clear();

    QTreeWidgetItem* scene_item = new QTreeWidgetItem(QStringList{"Scene"});
    scene_item->setFlags(scene_item->flags() & ~Qt::ItemIsSelectable);

    QFont scene_font = scene_item->font(0);
    scene_font.setBold(true);
    scene_item->setFont(0, scene_font);

    tree_->addTopLevelItem(scene_item);
    scene_item->setExpanded(true);
}

/**
 * @brief Устанавливает Scene и немедленно синхронизирует дерево.
 *
 * @param scene Сцена, принадлежащая SceneViewport.
 */
void HierarchyPanel::SetScene(Scene* scene) {
    scene_ = scene;
    Refresh();
}

/**
 * @brief Перестраивает Hierarchy по текущему массиву SceneObject.
 *
 * Во время clear() и повторного заполнения сигналы QTreeWidget блокируются.
 * Это предотвращает ложный сброс selection в других частях редактора.
 */
void HierarchyPanel::Refresh() {
    const QSignalBlocker signal_blocker(tree_);

    tree_->clear();

    QTreeWidgetItem* scene_item = new QTreeWidgetItem(QStringList{"Scene"});
    scene_item->setFlags(scene_item->flags() & ~Qt::ItemIsSelectable);

    QFont scene_font = scene_item->font(0);
    scene_font.setBold(true);
    scene_item->setFont(0, scene_font);

    tree_->addTopLevelItem(scene_item);

    if (scene_ == nullptr) {
        scene_item->setExpanded(true);
        return;
    }

    const auto& objects = scene_->GetObjects();
    scene_item->setToolTip(0, QString("%1 object(s) in scene").arg(objects.size()));

    for (std::size_t index = 0; index < objects.size(); ++index) {
        const std::shared_ptr<SceneObject>& object = objects[index];

        if (!object) {
            continue;
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(
            QStringList{
                QString::fromStdString(object->GetName())
            }
        );

        /**
         * В TreeItem хранится только индекс SceneObject в Scene.
         * QTreeWidget не получает владение самим SceneObject.
         */
        item->setData(0, Qt::UserRole, static_cast<qulonglong>(index));
        item->setToolTip(0, GetSceneObjectTypeName(object->GetType()));

        scene_item->addChild(item);
    }

    scene_item->setExpanded(true);
}

/**
 * @brief Выделяет в Hierarchy объект, выбранный вне панели.
 *
 * @param object Выбранный объект либо nullptr.
 */
void HierarchyPanel::SetSelectedObject(const std::shared_ptr<SceneObject>& object) {
    const QSignalBlocker signal_blocker(tree_);

    if (!object) {
        tree_->clearSelection();
        tree_->setCurrentItem(nullptr);
        return;
    }

    if (scene_ == nullptr) {
        return;
    }

    const auto& objects = scene_->GetObjects();
    std::size_t object_index = objects.size();

    for (std::size_t index = 0; index < objects.size(); ++index) {
        if (objects[index] == object) {
            object_index = index;
            break;
        }
    }

    if (object_index >= objects.size()) {
        tree_->clearSelection();
        tree_->setCurrentItem(nullptr);
        return;
    }

    QTreeWidgetItem* scene_item = tree_->topLevelItem(0);

    if (scene_item == nullptr) {
        return;
    }

    for (int child_index = 0; child_index < scene_item->childCount(); ++child_index) {
        QTreeWidgetItem* item = scene_item->child(child_index);

        if (item == nullptr) {
            continue;
        }

        const QVariant object_index_data = item->data(0, Qt::UserRole);

        if (!object_index_data.isValid()) {
            continue;
        }

        const std::size_t stored_index = static_cast<std::size_t>(object_index_data.toULongLong());

        if (stored_index != object_index) {
            continue;
        }

        tree_->setCurrentItem(item);
        item->setSelected(true);
        tree_->scrollToItem(item, QAbstractItemView::PositionAtCenter);
        return;
    }
}

/**
 * @brief Устанавливает обработчик пользовательского выбора в Hierarchy.
 *
 * @param callback Callback, принимающий выбранный SceneObject или nullptr.
 */
void HierarchyPanel::SetSelectionChangedCallback(std::function<void(std::shared_ptr<SceneObject>)> callback) {
    selection_changed_callback_ = std::move(callback);
}

/**
 * @brief Передаёт наружу объект, соответствующий текущей строке дерева.
 *
 * Корневой узел Scene не содержит Qt::UserRole и поэтому не интерпретируется
 * как SceneObject.
 */
void HierarchyPanel::HandleSelectionChanged() {
    if (scene_ == nullptr || !selection_changed_callback_) {
        return;
    }

    QTreeWidgetItem* item = tree_->currentItem();

    if (item == nullptr) {
        selection_changed_callback_(nullptr);
        return;
    }

    const QVariant object_index_data = item->data(0, Qt::UserRole);

    if (!object_index_data.isValid()) {
        selection_changed_callback_(nullptr);
        return;
    }

    const std::size_t object_index = static_cast<std::size_t>(object_index_data.toULongLong());
    const auto& objects = scene_->GetObjects();

    if (object_index >= objects.size()) {
        selection_changed_callback_(nullptr);
        return;
    }

    selection_changed_callback_(objects[object_index]);
}
