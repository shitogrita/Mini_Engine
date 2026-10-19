#pragma once

#include <QMainWindow>

class QAction;
class QDockWidget;

class HierarchyPanel;
class InspectorPanel;
class TestsDialog;
class SceneViewport;

/**
 * @brief Главное окно Mini Engine Editor.
 *
 * EditorWindow отвечает за компоновку интерфейса редактора и связывает
 * пользовательские действия с SceneViewport и боковыми панелями.
 *
 * Основные элементы интерфейса:
 * - центральный SceneViewport;
 * - Hierarchy слева;
 * - Inspector справа;
 * - главное меню;
 * - Status Bar;
 * - отдельное окно TestsDialog для benchmark.
 *
 * Рендеринг, Scene и OpenGL-ресурсы принадлежат не EditorWindow,
 * а соответствующим подсистемам движка. EditorWindow выполняет роль
 * координатора между интерфейсом и SceneViewport.
 */
class EditorWindow final : public QMainWindow {
public:
    explicit EditorWindow(QWidget* parent = nullptr);

private:
    /** @brief Создаёт QAction и связывает их с обработчиками. */
    void CreateActions();

    /** @brief Формирует главное меню редактора. */
    void CreateMenuBar();

    /** @brief Создаёт Hierarchy и Inspector как QDockWidget. */
    void CreateDockWidgets();

    /** @brief Создаёт строку состояния редактора. */
    void CreateStatusBar();

    /** @brief Настраивает главное окно и центральный SceneViewport. */
    void ConfigureWindow();

    /** @brief Применяет единую нейтральную палитру интерфейса в серых тонах. */
    void ApplyEditorStyle();

    /** @brief Открывает Wavefront OBJ и добавляет модель в текущую Scene. */
    void OpenModelFile();

    /** @brief Сохраняет текущую Scene в файл формата Mini Engine Scene. */
    void SaveScene();

    /** @brief Загружает Scene из файла формата Mini Engine Scene. */
    void OpenScene();

    /** @brief После подтверждения полностью очищает текущую Scene. */
    void ClearScene();

    /** @brief Показывает окно нагрузочных тестов и benchmark. */
    void ShowTestsDialog();

private:
    // File.
    QAction* open_model_action_ = nullptr;
    QAction* exit_action_ = nullptr;
    QAction* open_scene_action_ = nullptr;
    QAction* save_scene_action_ = nullptr;

    // Edit.
    QAction* undo_action_ = nullptr;
    QAction* redo_action_ = nullptr;

    // Scene / Create.
    QAction* create_cube_action_ = nullptr;
    QAction* create_plane_action_ = nullptr;
    QAction* create_sphere_action_ = nullptr;
    QAction* create_light_action_ = nullptr;
    QAction* clear_scene_action_ = nullptr;

    // Window / Tests / Help.
    QAction* show_hierarchy_action_ = nullptr;
    QAction* show_inspector_action_ = nullptr;
    QAction* open_tests_action_ = nullptr;
    QAction* about_action_ = nullptr;

    // Dock widgets.
    QDockWidget* hierarchy_dock_ = nullptr;
    QDockWidget* inspector_dock_ = nullptr;

    // Editor panels.
    HierarchyPanel* hierarchy_panel_ = nullptr;
    InspectorPanel* inspector_panel_ = nullptr;
    TestsDialog* tests_dialog_ = nullptr;

    // Центральная 3D-сцена редактора.
    SceneViewport* scene_viewport_ = nullptr;
};
