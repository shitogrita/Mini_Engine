#pragma once

#include <QMainWindow>

class QAction;
class QDockWidget;

class HierarchyPanel;
class InspectorPanel;
class ProjectPanel;
class SceneViewport;

class EditorWindow final : public QMainWindow {
public:
	explicit EditorWindow(QWidget* parent = nullptr);

private:
	void CreateActions();
	void CreateMenuBar();
	void CreateDockWidgets();
	void CreateStatusBar();
	void ConfigureWindow();
	void ApplyEditorStyle();

	void OpenModelFile();

	QAction* open_model_action_ = nullptr;
	QAction* exit_action_ = nullptr;

	QAction* undo_action_ = nullptr;
	QAction* redo_action_ = nullptr;

	QAction* import_asset_action_ = nullptr;
	QAction* create_material_action_ = nullptr;

	QAction* create_empty_action_ = nullptr;
	QAction* create_cube_action_ = nullptr;
	QAction* create_light_action_ = nullptr;

	QAction* show_hierarchy_action_ = nullptr;
	QAction* show_inspector_action_ = nullptr;
	QAction* show_project_action_ = nullptr;

	QAction* about_action_ = nullptr;

	QDockWidget* hierarchy_dock_ = nullptr;
	QDockWidget* inspector_dock_ = nullptr;
	QDockWidget* project_dock_ = nullptr;

	HierarchyPanel* hierarchy_panel_ = nullptr;
	InspectorPanel* inspector_panel_ = nullptr;
	ProjectPanel* project_panel_ = nullptr;
	SceneViewport* scene_viewport_ = nullptr;
};