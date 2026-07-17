#include "Editor/EditorWindow.h"

#include "Editor/Panels/HierarchyPanel.h"
#include "Editor/Panels/InspectorPanel.h"
#include "Editor/Panels/ProjectPanel.h"
#include "Editor/Viewport/SceneViewport.h"

#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

EditorWindow::EditorWindow(QWidget* parent)
    : QMainWindow(parent)
{
    ConfigureWindow();
    CreateActions();
    CreateMenuBar();
    CreateDockWidgets();
    CreateStatusBar();
    ApplyEditorStyle();
}

void EditorWindow::ConfigureWindow()
{
    setWindowTitle("Mini Engine Editor");
    resize(1440, 900);

    setDockNestingEnabled(true);
    setDockOptions(
        QMainWindow::AllowNestedDocks |
        QMainWindow::AllowTabbedDocks |
        QMainWindow::AnimatedDocks
    );

    scene_viewport_ = new SceneViewport(this);
    setCentralWidget(scene_viewport_);
}

void EditorWindow::CreateActions()
{
    open_model_action_ = new QAction("Open Model...", this);
    open_model_action_->setShortcut(
        QKeySequence::Open
    );

    exit_action_ = new QAction("Exit", this);
    exit_action_->setShortcut(
        QKeySequence::Quit
    );

    undo_action_ = new QAction("Undo", this);
    undo_action_->setShortcut(
        QKeySequence::Undo
    );

    redo_action_ = new QAction("Redo", this);
    redo_action_->setShortcut(
        QKeySequence::Redo
    );

    import_asset_action_ = new QAction("Import Asset...", this);
    create_material_action_ = new QAction("Create Material", this);

    create_empty_action_ = new QAction("Create Empty", this);
    create_cube_action_ = new QAction("Create Cube", this);
    create_light_action_ = new QAction("Create Light", this);

    show_hierarchy_action_ = new QAction("Hierarchy", this);
    show_inspector_action_ = new QAction("Inspector", this);
    show_project_action_ = new QAction("Project", this);

    show_hierarchy_action_->setCheckable(true);
    show_inspector_action_->setCheckable(true);
    show_project_action_->setCheckable(true);

    show_hierarchy_action_->setChecked(true);
    show_inspector_action_->setChecked(true);
    show_project_action_->setChecked(true);

    about_action_ = new QAction("About", this);

    connect(
        open_model_action_,
        &QAction::triggered,
        this,
        &EditorWindow::OpenModelFile
    );

    connect(
        exit_action_,
        &QAction::triggered,
        this,
        &QWidget::close
    );

    connect(
        about_action_,
        &QAction::triggered,
        this,
        [this]()
        {
            QMessageBox::about(
                this,
                "About Mini Engine Editor",
                "Mini Engine Editor\n"
                "Qt frontend for the game engine."
            );
        }
    );
}

void EditorWindow::CreateMenuBar()
{
    QMenu* file_menu = menuBar()->addMenu("File");
    file_menu->addAction(open_model_action_);
    file_menu->addSeparator();
    file_menu->addAction(exit_action_);

    QMenu* edit_menu = menuBar()->addMenu("Edit");
    edit_menu->addAction(undo_action_);
    edit_menu->addAction(redo_action_);

    QMenu* assets_menu = menuBar()->addMenu("Assets");
    assets_menu->addAction(import_asset_action_);
    assets_menu->addAction(create_material_action_);

    QMenu* game_object_menu = menuBar()->addMenu("GameObject");
    game_object_menu->addAction(create_empty_action_);
    game_object_menu->addAction(create_cube_action_);
    game_object_menu->addAction(create_light_action_);

    QMenu* window_menu = menuBar()->addMenu("Window");
    window_menu->addAction(show_hierarchy_action_);
    window_menu->addAction(show_inspector_action_);
    window_menu->addAction(show_project_action_);

    QMenu* help_menu = menuBar()->addMenu("Help");
    help_menu->addAction(about_action_);
}

void EditorWindow::CreateDockWidgets()
{
    hierarchy_dock_ = new QDockWidget("Hierarchy", this);
    hierarchy_dock_->setObjectName("HierarchyDock");

    hierarchy_panel_ = new HierarchyPanel(hierarchy_dock_);
    hierarchy_dock_->setWidget(hierarchy_panel_);

    addDockWidget(
        Qt::LeftDockWidgetArea,
        hierarchy_dock_
    );

    inspector_dock_ = new QDockWidget("Inspector", this);
    inspector_dock_->setObjectName("InspectorDock");

    inspector_panel_ = new InspectorPanel(inspector_dock_);
    inspector_dock_->setWidget(inspector_panel_);

    addDockWidget(
        Qt::RightDockWidgetArea,
        inspector_dock_
    );

    project_dock_ = new QDockWidget("Project", this);
    project_dock_->setObjectName("ProjectDock");

    project_panel_ = new ProjectPanel(project_dock_);
    project_dock_->setWidget(project_panel_);

    addDockWidget(
        Qt::BottomDockWidgetArea,
        project_dock_
    );

    resizeDocks(
        {hierarchy_dock_, inspector_dock_},
        {260, 300},
        Qt::Horizontal
    );

    resizeDocks(
        {project_dock_},
        {230},
        Qt::Vertical
    );

    connect(
        show_hierarchy_action_,
        &QAction::toggled,
        hierarchy_dock_,
        &QDockWidget::setVisible
    );

    connect(
        show_inspector_action_,
        &QAction::toggled,
        inspector_dock_,
        &QDockWidget::setVisible
    );

    connect(
        show_project_action_,
        &QAction::toggled,
        project_dock_,
        &QDockWidget::setVisible
    );

    connect(
        hierarchy_dock_,
        &QDockWidget::visibilityChanged,
        show_hierarchy_action_,
        &QAction::setChecked
    );

    connect(
        inspector_dock_,
        &QDockWidget::visibilityChanged,
        show_inspector_action_,
        &QAction::setChecked
    );

    connect(
        project_dock_,
        &QDockWidget::visibilityChanged,
        show_project_action_,
        &QAction::setChecked
    );
}

void EditorWindow::CreateStatusBar()
{
    statusBar()->showMessage("Ready");
}

void EditorWindow::OpenModelFile()
{
    const QString file_path =
        QFileDialog::getOpenFileName(
            this,
            "Open model",
            QString(),
            "3D models (*.obj);;Wavefront OBJ (*.obj);;All files (*.*)"
        );

    if (file_path.isEmpty()) {
        return;
    }

    const QFileInfo file_info(file_path);

    scene_viewport_->SetDisplayedFile(file_path);
    project_panel_->AddImportedFile(file_path);

    statusBar()->showMessage(
        "Selected model: " + file_info.fileName()
    );
}

void EditorWindow::ApplyEditorStyle()
{
    setStyleSheet(
        R"(
            QMainWindow {
                background-color: #1e1e1e;
            }

            QMenuBar {
                background-color: #2b2b2b;
                color: #dddddd;
                border-bottom: 1px solid #3a3a3a;
                padding: 2px;
            }

            QMenuBar::item {
                background-color: transparent;
                padding: 5px 10px;
            }

            QMenuBar::item:selected {
                background-color: #3d3d3d;
            }

            QMenu {
                background-color: #2b2b2b;
                color: #dddddd;
                border: 1px solid #444444;
            }

            QMenu::item {
                padding: 6px 28px 6px 20px;
            }

            QMenu::item:selected {
                background-color: #3f4f5f;
            }

            QDockWidget {
                color: #dddddd;
                font-weight: 600;
                titlebar-close-icon: none;
                titlebar-normal-icon: none;
            }

            QDockWidget::title {
                background-color: #292929;
                border: 1px solid #3a3a3a;
                padding: 6px;
                text-align: left;
            }

            QTreeWidget {
                background-color: #252525;
                color: #dddddd;
                border: none;
                outline: none;
                alternate-background-color: #282828;
            }

            QTreeWidget::item {
                min-height: 23px;
            }

            QTreeWidget::item:selected {
                background-color: #3f5264;
            }

            QLabel {
                color: #dddddd;
            }

            QLineEdit {
                background-color: #202020;
                color: #dddddd;
                border: 1px solid #454545;
                border-radius: 3px;
                padding: 5px;
            }

            QStatusBar {
                background-color: #292929;
                color: #bbbbbb;
                border-top: 1px solid #3a3a3a;
            }

            QSplitter::handle {
                background-color: #3a3a3a;
            }
        )"
    );
}