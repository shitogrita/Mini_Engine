#include "Editor/Panels/HierarchyPanel.h"

#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <cstdint>
#include <utility>


HierarchyPanel::HierarchyPanel(
    QWidget* parent
)
    : QWidget(parent)
{
    CreateLayout();

    FillPlaceholderScene();
}


void HierarchyPanel::CreateLayout()
{
    QVBoxLayout* layout =
        new QVBoxLayout(this);


    layout->setContentsMargins(
        0,
        0,
        0,
        0
    );


    layout->setSpacing(
        0
    );


    tree_ =
        new QTreeWidget(this);


    tree_->setHeaderHidden(
        true
    );


    tree_->setAlternatingRowColors(
        false
    );


    tree_->setIndentation(
        18
    );


    layout->addWidget(
        tree_
    );


    connect(
        tree_,
        &QTreeWidget::itemSelectionChanged,
        this,
        [this]()
        {
            HandleSelectionChanged();
        }
    );
}


void HierarchyPanel::FillPlaceholderScene()
{
    tree_->clear();


    QTreeWidgetItem* scene_item =
        new QTreeWidgetItem(
            QStringList{
                "Sample Scene"
            }
        );


    QTreeWidgetItem* camera_item =
        new QTreeWidgetItem(
            QStringList{
                "Main Camera"
            }
        );


    QTreeWidgetItem* light_item =
        new QTreeWidgetItem(
            QStringList{
                "Directional Light"
            }
        );


    QTreeWidgetItem* cube_item =
        new QTreeWidgetItem(
            QStringList{
                "Cube"
            }
        );


    scene_item->addChild(
        camera_item
    );


    scene_item->addChild(
        light_item
    );


    scene_item->addChild(
        cube_item
    );


    tree_->addTopLevelItem(
        scene_item
    );


    scene_item->setExpanded(
        true
    );
}


void HierarchyPanel::SetScene(
    Scene* scene
)
{
    scene_ =
        scene;


    Refresh();
}


void HierarchyPanel::Refresh()
{
    /*
     * Во время полной перестройки TreeWidget
     * временно блокируем сигналы.
     *
     * Иначе clear() может вызвать
     * itemSelectionChanged и неожиданно
     * сбросить selection в остальных панелях.
     */
    const QSignalBlocker signal_blocker(
        tree_
    );


    tree_->clear();


    QTreeWidgetItem* scene_item =
        new QTreeWidgetItem(
            QStringList{
                "Scene"
            }
        );


    tree_->addTopLevelItem(
        scene_item
    );


    if (scene_ == nullptr) {
        scene_item->setExpanded(
            true
        );

        return;
    }


    const auto& objects =
        scene_->GetObjects();


    for (
        std::size_t index = 0;
        index < objects.size();
        ++index
    ) {
        const std::shared_ptr<SceneObject>& object =
            objects[index];


        if (!object) {
            continue;
        }


        QTreeWidgetItem* item =
            new QTreeWidgetItem(
                QStringList{
                    QString::fromStdString(
                        object->GetName()
                    )
                }
            );


        /*
         * В TreeItem хранится индекс SceneObject
         * внутри Scene.
         *
         * UI не становится владельцем объекта.
         */
        item->setData(
            0,
            Qt::UserRole,
            static_cast<qulonglong>(
                index
            )
        );


        scene_item->addChild(
            item
        );
    }


    scene_item->setExpanded(
        true
    );
}


void HierarchyPanel::SetSelectedObject(
    const std::shared_ptr<SceneObject>& object
)
{
    /*
     * Этот метод вызывается извне,
     * например SceneViewport.
     *
     * При программном выборе строки
     * блокируем сигнал itemSelectionChanged,
     * чтобы не создать цикл:
     *
     * Viewport
     * -> Hierarchy
     * -> callback
     * -> Viewport
     * -> ...
     */
    const QSignalBlocker signal_blocker(
        tree_
    );


    /*
     * nullptr означает:
     * selection должен быть снят.
     */
    if (!object) {
        tree_->clearSelection();

        tree_->setCurrentItem(
            nullptr
        );

        return;
    }


    if (scene_ == nullptr) {
        return;
    }


    const auto& objects =
        scene_->GetObjects();


    /*
     * Находим индекс того же SceneObject.
     *
     * Сравниваем shared_ptr:
     * нам нужен именно тот объект,
     * который находится в Scene.
     */
    std::size_t object_index =
        objects.size();


    for (
        std::size_t index = 0;
        index < objects.size();
        ++index
    ) {
        if (
            objects[index] ==
            object
        ) {
            object_index =
                index;

            break;
        }
    }


    /*
     * Объект не принадлежит текущей Scene.
     */
    if (
        object_index >=
        objects.size()
    ) {
        tree_->clearSelection();

        tree_->setCurrentItem(
            nullptr
        );

        return;
    }


    /*
     * Верхний элемент — это корень Scene.
     */
    QTreeWidgetItem* scene_item =
        tree_->topLevelItem(
            0
        );


    if (scene_item == nullptr) {
        return;
    }


    /*
     * Ищем TreeItem,
     * чей Qt::UserRole содержит индекс объекта.
     */
    for (
        int child_index = 0;
        child_index <
            scene_item->childCount();
        ++child_index
    ) {
        QTreeWidgetItem* item =
            scene_item->child(
                child_index
            );


        if (item == nullptr) {
            continue;
        }


        const QVariant object_index_data =
            item->data(
                0,
                Qt::UserRole
            );


        if (!object_index_data.isValid()) {
            continue;
        }


        const std::size_t stored_index =
            static_cast<std::size_t>(
                object_index_data.toULongLong()
            );


        if (
            stored_index !=
            object_index
        ) {
            continue;
        }


        /*
         * Выделяем найденную строку.
         */
        tree_->setCurrentItem(
            item
        );


        item->setSelected(
            true
        );


        /*
         * Если Hierarchy станет длинной,
         * выбранный объект автоматически
         * прокрутится в видимую область.
         */
        tree_->scrollToItem(
            item
        );


        return;
    }
}


void HierarchyPanel::SetSelectionChangedCallback(
    std::function<void(
        std::shared_ptr<SceneObject>
    )> callback
)
{
    selection_changed_callback_ =
        std::move(
            callback
        );
}


void HierarchyPanel::HandleSelectionChanged()
{
    if (
        scene_ == nullptr ||
        !selection_changed_callback_
    ) {
        return;
    }


    QTreeWidgetItem* item =
        tree_->currentItem();


    if (item == nullptr) {
        selection_changed_callback_(
            nullptr
        );

        return;
    }


    /*
     * Корневой Scene item не содержит
     * индекса SceneObject.
     */
    const QVariant object_index_data =
        item->data(
            0,
            Qt::UserRole
        );


    if (!object_index_data.isValid()) {
        selection_changed_callback_(
            nullptr
        );

        return;
    }


    const std::size_t object_index =
        static_cast<std::size_t>(
            object_index_data.toULongLong()
        );


    const auto& objects =
        scene_->GetObjects();


    if (
        object_index >=
        objects.size()
    ) {
        selection_changed_callback_(
            nullptr
        );

        return;
    }


    selection_changed_callback_(
        objects[object_index]
    );
}