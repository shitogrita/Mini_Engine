#pragma once

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneObject.h"

#include <QWidget>

#include <functional>
#include <memory>

class QTreeWidget;
class QTreeWidgetItem;

/**
 * @brief Панель иерархии объектов текущей сцены.
 *
 * HierarchyPanel отображает содержимое Scene в виде дерева и является
 * одной из двух точек выбора SceneObject в редакторе наряду с SceneViewport.
 * Панель не владеет объектами сцены: в элементах QTreeWidget хранится только
 * индекс соответствующего SceneObject внутри Scene.
 *
 * Синхронизация выбора строится в двух направлениях:
 * - выбор строки в Hierarchy передаётся наружу через callback;
 * - выбор объекта во Viewport отражается через SetSelectedObject().
 *
 * При программном обновлении дерева и selection сигналы QTreeWidget
 * блокируются, чтобы исключить рекурсивный цикл между Hierarchy,
 * SceneViewport и Inspector.
 */
class HierarchyPanel final : public QWidget {
public:
    explicit HierarchyPanel(QWidget* parent = nullptr);

    /**
     * @brief Устанавливает сцену, отображаемую в Hierarchy.
     *
     * Scene остаётся во владении SceneViewport. HierarchyPanel хранит только
     * невладеющий указатель и после установки сразу перестраивает дерево.
     *
     * @param scene Сцена редактора или nullptr.
     */
    void SetScene(Scene* scene);

    /**
     * @brief Полностью перестраивает дерево по текущему содержимому Scene.
     *
     * Метод используется после создания, удаления, импорта, очистки или
     * загрузки объектов сцены.
     */
    void Refresh();

    /**
     * @brief Отражает внешний выбор SceneObject внутри Hierarchy.
     *
     * Метод не вызывает selection callback. Это позволяет безопасно
     * синхронизировать выбор из SceneViewport без обратного рекурсивного
     * вызова Hierarchy -> Viewport -> Hierarchy.
     *
     * @param object Выбранный объект или nullptr для снятия selection.
     */
    void SetSelectedObject(const std::shared_ptr<SceneObject>& object);

    /**
     * @brief Устанавливает callback выбора объекта пользователем.
     *
     * @param callback Функция, получающая выбранный SceneObject либо nullptr.
     */
    void SetSelectionChangedCallback(std::function<void(std::shared_ptr<SceneObject>)> callback);

private:
    /**
     * @brief Создаёт и настраивает дерево Hierarchy.
     */
    void CreateLayout();

    /**
     * @brief Создаёт пустой корневой элемент до привязки реальной Scene.
     */
    void FillPlaceholderScene();

    /**
     * @brief Обрабатывает пользовательское изменение selection в QTreeWidget.
     */
    void HandleSelectionChanged();

private:
    QTreeWidget* tree_ = nullptr;
    Scene* scene_ = nullptr;
    std::function<void(std::shared_ptr<SceneObject>)> selection_changed_callback_;
};
