#include "Editor/EditorWindow.h"

#include "Editor/Panels/HierarchyPanel.h"
#include "Editor/Panels/InspectorPanel.h"
#include "Editor/Panels/TestsDialog.h"
#include "Editor/Viewport/SceneViewport.h"

#include "Engine/Scene/SceneSerializer.h"

#include <QAction>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QUrl>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <utility>

/**
 * @brief Создаёт главное окно Mini Engine Editor.
 *
 * Инициализация выполняется по этапам:
 * - настройка QMainWindow и SceneViewport;
 * - создание QAction;
 * - построение главного меню;
 * - создание Hierarchy и Inspector;
 * - создание Status Bar;
 * - применение общего визуального стиля.
 *
 * @param parent Родительский QWidget.
 */
EditorWindow::EditorWindow(QWidget* parent) : QMainWindow(parent) {
    ConfigureWindow();
    CreateActions();
    CreateMenuBar();
    CreateDockWidgets();
    CreateStatusBar();
    ApplyEditorStyle();
}

/**
 * @brief Настраивает главное окно редактора.
 *
 * SceneViewport является центральным widget окна и занимает всё пространство,
 * не занятое Hierarchy и Inspector.
 */
void EditorWindow::ConfigureWindow() {
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

/**
 * @brief Создаёт QAction, используемые главным меню Editor.
 *
 * В интерфейсе оставлены только действия, связанные с текущей реализацией:
 * работа со Scene, создание примитивов и Point Light, управление панелями,
 * открытие benchmark и информация о приложении.
 */
void EditorWindow::CreateActions() {
    open_model_action_ = new QAction("Open Model...", this);
    open_model_action_->setShortcut(QKeySequence::Open);

    save_scene_action_ = new QAction("Save Scene...", this);
    save_scene_action_->setShortcut(QKeySequence::Save);

    open_scene_action_ = new QAction("Open Scene...", this);

    exit_action_ = new QAction("Exit", this);
    exit_action_->setShortcut(QKeySequence::Quit);

    undo_action_ = new QAction("Undo", this);
    undo_action_->setShortcut(QKeySequence::Undo);

    redo_action_ = new QAction("Redo", this);
    redo_action_->setShortcut(QKeySequence::Redo);

    create_cube_action_ = new QAction("Cube", this);
    create_plane_action_ = new QAction("Plane", this);
    create_sphere_action_ = new QAction("Sphere", this);
    create_light_action_ = new QAction("Point Light", this);

    clear_scene_action_ = new QAction("Clear Scene", this);

    /**
     * Hierarchy и Inspector являются DockWidget,
     * поэтому их видимость управляется checkable QAction.
     */
    show_hierarchy_action_ = new QAction("Hierarchy", this);
    show_inspector_action_ = new QAction("Inspector", this);

    show_hierarchy_action_->setCheckable(true);
    show_inspector_action_->setCheckable(true);

    show_hierarchy_action_->setChecked(true);
    show_inspector_action_->setChecked(true);

    /**
     * Tests открывается отдельным окном,
     * а не дополнительной Dock-панелью Editor.
     */
    open_tests_action_ = new QAction("Open Tests...", this);

    about_action_ = new QAction("About", this);

    connect(open_model_action_, &QAction::triggered, this, &EditorWindow::OpenModelFile);
    connect(save_scene_action_, &QAction::triggered, this, &EditorWindow::SaveScene);
    connect(open_scene_action_, &QAction::triggered, this, &EditorWindow::OpenScene);
    connect(exit_action_, &QAction::triggered, this, &QWidget::close);
    connect(open_tests_action_, &QAction::triggered, this, &EditorWindow::ShowTestsDialog);
    connect(clear_scene_action_, &QAction::triggered, this, &EditorWindow::ClearScene);

    /**
     * После создания объекта обновляем Hierarchy и синхронизируем selection.
     * Сам SceneObject создаётся внутри SceneViewport, где находится Scene
     * и активный OpenGL Context.
     */
    connect(create_cube_action_, &QAction::triggered, this, [this]() {
        scene_viewport_->CreateCube();

        hierarchy_panel_->Refresh();
        hierarchy_panel_->SetSelectedObject(scene_viewport_->GetSelectedObject());
        inspector_panel_->SetSelectedObject(scene_viewport_->GetSelectedObject());
    });

    connect(create_plane_action_, &QAction::triggered, this, [this]() {
        scene_viewport_->CreatePlane();

        hierarchy_panel_->Refresh();
        hierarchy_panel_->SetSelectedObject(scene_viewport_->GetSelectedObject());
        inspector_panel_->SetSelectedObject(scene_viewport_->GetSelectedObject());
    });

    connect(create_sphere_action_, &QAction::triggered, this, [this]() {
        scene_viewport_->CreateSphere();

        hierarchy_panel_->Refresh();
        hierarchy_panel_->SetSelectedObject(scene_viewport_->GetSelectedObject());
        inspector_panel_->SetSelectedObject(scene_viewport_->GetSelectedObject());
    });

    connect(create_light_action_, &QAction::triggered, this, [this]() {
        scene_viewport_->CreatePointLight();

        hierarchy_panel_->Refresh();
        hierarchy_panel_->SetSelectedObject(scene_viewport_->GetSelectedObject());
        inspector_panel_->SetSelectedObject(scene_viewport_->GetSelectedObject());
    });

    connect(about_action_, &QAction::triggered, this, [this]() {
        QMessageBox::about(
            this,
            "About Mini Engine Editor",
            "Mini Engine Editor\nQt frontend for the game engine."
        );
    });
}

/**
 * @brief Создаёт главное меню Editor.
 *
 * Меню организовано по назначению:
 * - File — загрузка и сохранение;
 * - Edit — стандартные действия редактирования;
 * - Create — создание объектов сцены;
 * - Scene — операции над всей сценой;
 * - Window — управление видимостью боковых панелей;
 * - Tests — нагрузочные тесты и benchmark;
 * - Help — информация о приложении.
 */
void EditorWindow::CreateMenuBar() {
    menuBar()->setNativeMenuBar(true);

    QMenu* file_menu = menuBar()->addMenu("File");
    file_menu->addAction(open_scene_action_);
    file_menu->addAction(save_scene_action_);
    file_menu->addSeparator();
    file_menu->addAction(open_model_action_);
    file_menu->addSeparator();
    file_menu->addAction(exit_action_);

    QMenu* edit_menu = menuBar()->addMenu("Edit");
    edit_menu->addAction(undo_action_);
    edit_menu->addAction(redo_action_);

    QMenu* create_menu = menuBar()->addMenu("Create");
    create_menu->addAction(create_cube_action_);
    create_menu->addAction(create_plane_action_);
    create_menu->addAction(create_sphere_action_);
    create_menu->addSeparator();
    create_menu->addAction(create_light_action_);

    QMenu* scene_menu = menuBar()->addMenu("Scene");
    scene_menu->addAction(clear_scene_action_);

    QMenu* window_menu = menuBar()->addMenu("Window");
    window_menu->addAction(show_hierarchy_action_);
    window_menu->addAction(show_inspector_action_);

    QMenu* tests_menu = menuBar()->addMenu("Tests");
    tests_menu->addAction(open_tests_action_);

    QMenu* help_menu = menuBar()->addMenu("Help");
    help_menu->addAction(about_action_);
}

/**
 * @brief Создаёт основные DockWidget редактора.
 *
 * После удаления неиспользуемой Project-панели интерфейс состоит из:
 * - Hierarchy слева;
 * - SceneViewport по центру;
 * - Inspector справа.
 *
 * Здесь же настраивается синхронизация selection между Hierarchy,
 * SceneViewport и Inspector.
 */
void EditorWindow::CreateDockWidgets() {
    hierarchy_dock_ = new QDockWidget(
        "Hierarchy",
        this
    );

    hierarchy_dock_->setObjectName(
        "HierarchyDock"
    );

    hierarchy_dock_->setMinimumWidth(210);

    hierarchy_panel_ = new HierarchyPanel(
        hierarchy_dock_
    );

    hierarchy_dock_->setWidget(
        hierarchy_panel_
    );

    addDockWidget(
        Qt::LeftDockWidgetArea,
        hierarchy_dock_
    );

    inspector_dock_ = new QDockWidget(
        "Inspector",
        this
    );

    inspector_dock_->setObjectName(
        "InspectorDock"
    );

    inspector_dock_->setMinimumWidth(290);

    inspector_panel_ = new InspectorPanel(
        inspector_dock_
    );

    /**
     * Изменение параметров Point Light в Inspector
     * должно сразу быть видно в SceneViewport.
     */
    inspector_panel_->SetLightChangedCallback(
        [this]() {
            scene_viewport_->update();
        }
    );

    inspector_dock_->setWidget(
        inspector_panel_
    );

    addDockWidget(
        Qt::RightDockWidgetArea,
        inspector_dock_
    );

    /**
     * После удаления нижней Project-панели SceneViewport занимает
     * всю доступную высоту центральной области окна.
     */
    resizeDocks(
        {
            hierarchy_dock_,
            inspector_dock_
        },
        {
            230,
            320
        },
        Qt::Horizontal
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

    /**
     * Hierarchy отображает реальные SceneObject,
     * принадлежащие Scene внутри SceneViewport.
     */
    hierarchy_panel_->SetScene(
        &scene_viewport_->GetScene()
    );

    /**
     * Выбор объекта в Hierarchy:
     * 1. передаёт объект в SceneViewport;
     * 2. передаёт тот же объект в Inspector.
     */
    hierarchy_panel_->SetSelectionChangedCallback(
        [this](std::shared_ptr<SceneObject> object) {
            scene_viewport_->SetSelectedObject(
                object
            );

            inspector_panel_->SetSelectedObject(
                std::move(object)
            );
        }
    );

    /**
     * Выбор объекта непосредственно во Viewport:
     * 1. SceneViewport выполняет Ray Picking;
     * 2. Hierarchy выделяет найденный объект;
     * 3. Inspector показывает его параметры.
     */
    scene_viewport_->SetSelectionChangedCallback(
        [this](std::shared_ptr<SceneObject> object) {
            if (!object) {
                hierarchy_panel_->Refresh();
            }

            hierarchy_panel_->SetSelectedObject(
                object
            );

            inspector_panel_->SetSelectedObject(
                std::move(object)
            );
        }
    );

    /**
     * Inspector передаёт выбранный путь к текстуре в SceneViewport,
     * потому что создание Texture2D требует активного OpenGL Context.
     */
    inspector_panel_->SetTextureChangedCallback(
        [this](Material& material, const QString& file_path) {
            scene_viewport_->SetMaterialTexture(
                material,
                file_path
            );
        }
    );

    /**
     * Когда Transform меняется через Gizmo,
     * Inspector синхронизирует отображаемые значения.
     */
    scene_viewport_->SetTransformChangedCallback(
        [this]() {
            inspector_panel_->RefreshTransformFields();
        }
    );
}

/**
 * @brief Создаёт Status Bar.
 */
void EditorWindow::CreateStatusBar() {
    statusBar()->showMessage("Ready");
}

/**
 * @brief Открывает отдельное окно нагрузочных тестов.
 *
 * TestsDialog создаётся один раз. При повторном вызове уже существующее
 * окно поднимается поверх Editor и получает focus.
 *
 * Через callbacks окно связано с SceneViewport и получает:
 * - показатели производительности;
 * - состояние benchmark;
 * - прогресс полного benchmark;
 * - команды запуска и остановки тестов.
 */
void EditorWindow::ShowTestsDialog() {
    if (!tests_dialog_) {
        tests_dialog_ = new TestsDialog(this);

        scene_viewport_->SetPerformanceStatsCallback(
            [this](const PerformanceStats& stats) {
                if (!tests_dialog_) {
                    return;
                }

                tests_dialog_->SetPerformanceStats(
                    stats.object_count,
                    stats.worker_count,
                    stats.update_ms,
                    stats.render_ms,
                    stats.frame_ms,
                    stats.processing_fps,
                    stats.shadow_ms,
                    stats.main_ms,
                    stats.editor_ms,
                    stats.draw_calls,
                    stats.shadow_draw_calls,
                    stats.main_draw_calls,
                    stats.editor_draw_calls
                );
            }
        );

        scene_viewport_->SetBenchmarkSuiteProgressCallback(
            [this](std::size_t current, std::size_t total) {
                if (!tests_dialog_) {
                    return;
                }

                tests_dialog_->SetBenchmarkProgress(
                    current,
                    total
                );

                statusBar()->showMessage(
                    "Full benchmark: " +
                    QString::number(static_cast<qulonglong>(current)) +
                    " / " +
                    QString::number(static_cast<qulonglong>(total))
                );
            }
        );

        scene_viewport_->SetBenchmarkSuiteFinishedCallback(
            [this]() {
                if (!tests_dialog_) {
                    return;
                }

                tests_dialog_->SetRunning(
                    false
                );

                hierarchy_panel_->Refresh();

                hierarchy_panel_->SetSelectedObject(
                    nullptr
                );

                inspector_panel_->ClearSelection();

                /**
                 * Full Benchmark записывает итоговый CSV
                 * в текущий рабочий каталог приложения.
                 */
                const std::filesystem::path benchmark_file =
                    std::filesystem::current_path() /
                    "benchmark_full_results.csv";

                if (!std::filesystem::exists(benchmark_file)) {
                    statusBar()->showMessage(
                        "Full benchmark finished, but CSV file was not found.",
                        5000
                    );

                    return;
                }

                statusBar()->showMessage(
                    "Full benchmark finished. Opening benchmark_full_results.csv",
                    5000
                );

                /**
                 * CSV открывается приложением, назначенным операционной
                 * системой для файлов этого типа.
                 */
                const QString benchmark_file_path =
                    QString::fromStdString(
                        benchmark_file.string()
                    );

                const bool opened =
                    QDesktopServices::openUrl(
                        QUrl::fromLocalFile(
                            benchmark_file_path
                        )
                    );

                if (!opened) {
                    statusBar()->showMessage(
                        "Benchmark finished, but the CSV file could not be opened automatically.",
                        5000
                    );
                }
            }
        );

        /**
         * Запуск одного выбранного benchmark-case.
         */
        tests_dialog_->SetRunTestCallback(
            [this](TestScene test_scene, ExecutionMode execution_mode, std::size_t worker_count) {
                const bool started = scene_viewport_->StartTest(test_scene, execution_mode, worker_count);

                if (!started) {
                    QMessageBox::warning(
                        this,
                        "Tests",
                        "Failed to start test."
                    );

                    return;
                }

                hierarchy_panel_->Refresh();
                hierarchy_panel_->SetSelectedObject(nullptr);

                inspector_panel_->ClearSelection();

                tests_dialog_->SetRunning(true);

                /**
                 * Тестовая Scene является read-only.
                 * Боковые панели блокируются до Stop Test.
                 */
                hierarchy_panel_->setEnabled(false);
                inspector_panel_->setEnabled(false);

                statusBar()->showMessage(
                    "Benchmark test started",
                    2000
                );
            }
        );

        /**
         * Запуск полной автоматической матрицы benchmark.
         */
        tests_dialog_->SetRunFullBenchmarkCallback(
            [this]() {
                const bool started =
                    scene_viewport_->StartBenchmarkSuite();

                if (!started) {
                    QMessageBox::warning(
                        this,
                        "Tests",
                        "Failed to start full benchmark."
                    );

                    return;
                }

                hierarchy_panel_->Refresh();
                hierarchy_panel_->SetSelectedObject(nullptr);

                inspector_panel_->ClearSelection();

                tests_dialog_->SetRunning(true);

                statusBar()->showMessage(
                    "Full benchmark started"
                );
            }
        );

        /**
         * Останавливает одиночный тест или полную benchmark suite.
         */
        tests_dialog_->SetStopTestCallback(
            [this]() {
                if (scene_viewport_->IsBenchmarkSuiteRunning()) {
                    scene_viewport_->StopBenchmarkSuite();
                } else {
                    scene_viewport_->StopTest();
                    hierarchy_panel_->setEnabled(true);
                    inspector_panel_->setEnabled(true);
                }

                hierarchy_panel_->Refresh();

                hierarchy_panel_->SetSelectedObject(nullptr);

                inspector_panel_->ClearSelection();

                tests_dialog_->SetRunning(false);

                tests_dialog_->ResetPerformanceStats();

                statusBar()->showMessage(
                    "Test stopped",
                    2000
                );
            }
        );
    }

    tests_dialog_->show();
    tests_dialog_->raise();
    tests_dialog_->activateWindow();
}

/**
 * @brief Открывает Wavefront OBJ-модель и добавляет её в текущую Scene.
 */
void EditorWindow::OpenModelFile() {
    const QString file_path = QFileDialog::getOpenFileName(
        this,
        "Open model",
        QString(),
        "3D models (*.obj);;Wavefront OBJ (*.obj);;All files (*.*)"
    );

    if (file_path.isEmpty()) {
        return;
    }

    const QFileInfo file_info(
        file_path
    );

    scene_viewport_->SetDisplayedFile(
        file_path
    );

    hierarchy_panel_->Refresh();

    statusBar()->showMessage(
        "Selected model: " + file_info.fileName()
    );
}

/**
 * @brief Полностью очищает текущую Scene после подтверждения пользователя.
 *
 * После очистки selection сбрасывается одновременно в SceneViewport,
 * Hierarchy и Inspector.
 */
void EditorWindow::ClearScene() {
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        "Clear Scene",
        "Delete all objects from the scene?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (answer != QMessageBox::Yes) {
        return;
    }

    scene_viewport_->ClearScene();

    hierarchy_panel_->Refresh();

    hierarchy_panel_->SetSelectedObject(
        nullptr
    );

    inspector_panel_->SetSelectedObject(
        nullptr
    );

    statusBar()->showMessage(
        "Scene cleared"
    );
}

/**
 * @brief Сохраняет текущую Scene в файл.
 *
 * Если пользователь не указал расширение .scene,
 * оно добавляется автоматически.
 */
void EditorWindow::SaveScene() {
    QString file_path = QFileDialog::getSaveFileName(
        this,
        "Save Scene",
        "scene.scene",
        "Mini Engine Scene (*.scene)"
    );

    if (file_path.isEmpty()) {
        return;
    }

    if (!file_path.endsWith(".scene", Qt::CaseInsensitive)) {
        file_path += ".scene";
    }

    const bool saved = SceneSerializer::Save(
        scene_viewport_->GetScene(),
        file_path.toStdString()
    );

    if (!saved) {
        QMessageBox::warning(
            this,
            "Save Scene",
            "Failed to save scene."
        );

        return;
    }

    statusBar()->showMessage(
        "Scene saved: " + QFileInfo(file_path).fileName(),
        3000
    );
}

/**
 * @brief Загружает Scene из файла.
 *
 * SceneViewport выполняет фактическую загрузку и восстановление
 * runtime-ресурсов. После успешной загрузки Hierarchy обновляется,
 * а текущее выделение сбрасывается.
 */
void EditorWindow::OpenScene() {
    const QString file_path = QFileDialog::getOpenFileName(
        this,
        "Open Scene",
        QString(),
        "Mini Engine Scene (*.scene)"
    );

    if (file_path.isEmpty()) {
        return;
    }

    const bool loaded = scene_viewport_->LoadScene(
        file_path
    );

    if (!loaded) {
        QMessageBox::warning(
            this,
            "Open Scene",
            "Failed to open scene."
        );

        return;
    }

    hierarchy_panel_->Refresh();

    hierarchy_panel_->SetSelectedObject(
        nullptr
    );

    inspector_panel_->SetSelectedObject(
        nullptr
    );

    statusBar()->showMessage(
        "Scene opened: " + QFileInfo(file_path).fileName(),
        3000
    );
}

/**
 * @brief Устанавливает общий визуальный стиль Mini Engine Editor.
 */
void EditorWindow::ApplyEditorStyle() {
    /**
     * Общая палитра Editor построена на нейтральных серых оттенках.
     * Локальные панели могут уточнять отдельные элементы, но используют
     * те же уровни фона, границ и текста.
     */
    setStyleSheet(
        R"(
            QMainWindow {
                background-color: #1f2124;
            }

            QWidget {
                color: #d4d7db;
                font-size: 13px;
            }

            QMenuBar {
                background-color: #292b2f;
                color: #d4d7db;
                border-bottom: 1px solid #3a3d42;
                padding: 2px 5px;
            }

            QMenuBar::item {
                background-color: transparent;
                padding: 6px 10px;
                border-radius: 4px;
            }

            QMenuBar::item:selected {
                background-color: #383b40;
            }

            QMenu {
                background-color: #2a2c30;
                color: #d7d9dd;
                border: 1px solid #44474c;
                padding: 5px;
            }

            QMenu::item {
                padding: 6px 30px 6px 20px;
                border-radius: 4px;
            }

            QMenu::item:selected {
                background-color: #45494f;
                color: #ffffff;
            }

            QMenu::separator {
                height: 1px;
                background-color: #41444a;
                margin: 5px 8px;
            }

            QToolBar {
                background-color: #292b2f;
                border: none;
                border-bottom: 1px solid #3a3d42;
                spacing: 5px;
                padding: 4px 6px;
            }

            QToolButton {
                background-color: #34373c;
                color: #d7d9dd;
                border: 1px solid #474b51;
                border-radius: 4px;
                padding: 5px 11px;
            }

            QToolButton:hover {
                background-color: #3c4045;
                border-color: #5a5f66;
            }

            QToolButton:pressed,
            QToolButton:checked {
                background-color: #4a4e54;
                border-color: #666b73;
                color: #ffffff;
            }

            QDockWidget {
                color: #cfd2d6;
                font-weight: 500;
            }

            QDockWidget::title {
                background-color: #292b2f;
                border-bottom: 1px solid #3a3d42;
                padding: 6px 8px;
                text-align: left;
            }

            QDockWidget QWidget {
                background-color: #232529;
                color: #d4d7db;
            }

            QLabel {
                background-color: transparent;
                color: #d4d7db;
            }

            QLineEdit {
                background-color: #1f2124;
                color: #d8dade;
                border: 1px solid #41454b;
                border-radius: 4px;
                padding: 5px 7px;
                selection-background-color: #51555c;
                selection-color: #ffffff;
            }

            QLineEdit:hover {
                border-color: #555a61;
            }

            QLineEdit:focus {
                border-color: #6a6f77;
            }

            QDoubleSpinBox {
                background-color: #1f2124;
                color: #d8dade;
                border: 1px solid #41454b;
                border-radius: 4px;
                padding: 4px 6px;
                min-height: 22px;
                selection-background-color: #51555c;
                selection-color: #ffffff;
            }

            QDoubleSpinBox:hover {
                border-color: #555a61;
            }

            QDoubleSpinBox:focus {
                border-color: #6a6f77;
            }

            QDoubleSpinBox::up-button,
            QDoubleSpinBox::down-button {
                width: 0px;
                height: 0px;
                border: none;
            }

            QComboBox {
                background-color: #1f2124;
                color: #d8dade;
                border: 1px solid #41454b;
                border-radius: 4px;
                padding: 5px 8px;
                min-height: 22px;
            }

            QComboBox:hover {
                border-color: #555a61;
            }

            QComboBox:focus {
                border-color: #6a6f77;
            }

            QComboBox::drop-down {
                border: none;
                width: 22px;
            }

            QComboBox QAbstractItemView {
                background-color: #292b2f;
                color: #d8dade;
                border: 1px solid #44484e;
                selection-background-color: #4a4e54;
                selection-color: #ffffff;
                outline: none;
            }

            QPushButton {
                background-color: #34373c;
                color: #d8dade;
                border: 1px solid #484c52;
                border-radius: 4px;
                padding: 5px 11px;
                min-height: 23px;
            }

            QPushButton:hover {
                background-color: #3c4045;
                border-color: #5a5f66;
            }

            QPushButton:pressed {
                background-color: #2d3034;
            }

            QPushButton:disabled {
                color: #71757b;
                background-color: #292b2f;
                border-color: #393c41;
            }

            QCheckBox {
                color: #d1d4d8;
                spacing: 7px;
            }


            QStatusBar {
                background-color: #292b2f;
                color: #8f949b;
                border-top: 1px solid #3a3d42;
                min-height: 20px;
            }

            QSplitter::handle {
                background-color: #3a3d42;
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

            QScrollBar:horizontal {
                background-color: #232529;
                height: 10px;
            }

            QScrollBar::handle:horizontal {
                background-color: #4a4e54;
                border-radius: 5px;
                min-width: 28px;
            }

            QScrollBar::handle:horizontal:hover {
                background-color: #5a5f66;
            }

            QScrollBar::add-line:horizontal,
            QScrollBar::sub-line:horizontal {
                width: 0px;
            }

            QToolTip {
                background-color: #34373c;
                color: #eceef0;
                border: 1px solid #565a61;
                padding: 5px 7px;
            }
        )"
    );
}
