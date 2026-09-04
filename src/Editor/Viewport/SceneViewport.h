#pragma once

#include "Engine/Assets/ImportedMeshData.h"
#include "Engine/Math/Ray.h"
#include "Engine/Math/matrix_types.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneObject.h"

#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QPointF>
#include <QString>
#include <QTimer>

#include <functional>
#include <memory>
#include <optional>


class QLabel;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;


/**
 * @brief OpenGL viewport редактора.
 *
 * SceneViewport отвечает за:
 * - отображение сцены;
 * - управление камерой;
 * - импорт и отображение моделей;
 * - выбор объектов мышью;
 * - отображение editor grid и координатных осей;
 * - отображение выбранного SceneObject;
 * - Frame Selected;
 * - editor gizmo.
 */
class SceneViewport final : public QOpenGLWidget {
public:

    /**
     * @brief Режим проекции камеры.
     */
    enum class ProjectionMode {
        Perspective,
        Orthographic
    };


    /**
     * @brief Callback изменения выбранного объекта.
     *
     * Используется для синхронизации:
     *
     * Viewport -> Hierarchy -> Inspector.
     */
    using SelectionChangedCallback =
        std::function<void(
            std::shared_ptr<SceneObject>
        )>;

    using TransformChangedCallback =
        std::function<void()>;

    explicit SceneViewport(
        QWidget* parent = nullptr
    );

    void SetTransformChangedCallback (TransformChangedCallback callback);

    TransformChangedCallback transform_changed_callback_;

    ~SceneViewport() override;


    /**
     * @brief Передаёт путь к OBJ модели,
     * которая должна быть загружена в сцену.
     */
    void SetDisplayedFile(
        const QString& file_path
    );


    /**
     * @brief Устанавливает режим проекции.
     */
    void SetProjectionMode(
        ProjectionMode mode
    );


    /**
     * @brief Возвращает текущий режим проекции.
     */
    ProjectionMode GetProjectionMode() const;


    /**
     * @brief Возвращает сцену viewport.
     */
    Scene& GetScene();


    /**
     * @brief Возвращает сцену viewport
     * только для чтения.
     */
    const Scene& GetScene() const;


    /**
     * @brief Устанавливает выбранный SceneObject.
     *
     * Используется, когда объект выбирается
     * через HierarchyPanel.
     */
    void SetSelectedObject(
        std::shared_ptr<SceneObject> object
    );


    /**
     * @brief Возвращает выбранный SceneObject.
     */
    std::shared_ptr<SceneObject>
    GetSelectedObject() const;


    /**
     * @brief Устанавливает callback изменения selection.
     */
    void SetSelectionChangedCallback(
        SelectionChangedCallback callback
    );


private:

    /**
     * @brief Создаёт Qt-интерфейс viewport.
     */
    void CreateLayout();


    /**
     * @brief Загружает ожидающую OBJ модель
     * в OpenGL и создаёт SceneObject.
     */
    void UploadPendingMesh();


    /**
     * @brief Рассчитывает параметры отображения
     * импортированной модели.
     */
    void CalculateModelFit(
        const ImportedMeshData& mesh_data
    );


    /**
     * @brief Обрабатывает непрерывное
     * перемещение камеры.
     */
    void TickInput();


    /**
     * @brief Обновляет текст режима проекции.
     */
    void UpdateProjectionTitle();


    /**
     * @brief Обновляет информацию о камере
     * и выбранном объекте.
     */
    void UpdateCoordinatesLabel();


    /**
     * @brief Расставляет объекты сцены.
     *
     * Метод сохранён для возможного дальнейшего
     * использования, но при обычном импорте
     * объектов сейчас не вызывается.
     */
    void ArrangeSceneObjects();


    /**
     * @brief Создаёт Grid и мировые оси редактора.
     */
    void CreateEditorGrid();


    /**
     * @brief Создаёт геометрию Move Gizmo.
     *
     * Gizmo состоит из трёх локальных осей:
     *
     * X - красная;
     * Y - зелёная;
     * Z - синяя.
     */
    void CreateMoveGizmo();


    /**
     * @brief Создаёт геометрию сетки редактора.
     */
    ImportedMeshData
    CreateGridMeshData() const;

    enum class GizmoAxis {
        None,
        X,
        Y,
        Z
    };


    /**
     * @brief Создаёт линию между двумя точками.
     *
     * Используется для:
     * - мировых осей;
     * - Move Gizmo.
     */
    ImportedMeshData
    CreateAxisMeshData(
        const Vec3& start,
        const Vec3& end
    ) const;


    /**
     * @brief Рассчитывает позицию,
     * в которой должен появиться новый объект.
     */
    Vec3 FindSpawnPosition() const;


    /**
     * @brief Создаёт луч из позиции мыши
     * в пространство сцены.
     */
    Ray CreateMouseRay(
        const QPointF& mouse_position
    ) const;


    /**
     * @brief Выполняет Ray Picking
     * объектов сцены.
     */
    void SelectObjectAt(
        const QPointF& mouse_position
    );


    /**
     * @brief Уведомляет EditorWindow
     * об изменении выбранного объекта.
     */
    void NotifySelectionChanged();


    /**
     * @brief Перемещает камеру так,
     * чтобы выбранный объект оказался
     * в центре viewport.
     *
     * Вызывается клавишей F.
     */
    void FrameSelectedObject();


protected:

    /**
     * @brief Инициализация OpenGL ресурсов.
     */
    void initializeGL() override;


    /**
     * @brief Обновление OpenGL viewport
     * при изменении размеров окна.
     */
    void resizeGL(
        int width,
        int height
    ) override;


    /**
     * @brief Основной проход рендера сцены.
     */
    void paintGL() override;


    /**
     * @brief Обработка нажатия клавиш.
     */
    void keyPressEvent(
        QKeyEvent* event
    ) override;


    /**
     * @brief Обработка отпускания клавиш.
     */
    void keyReleaseEvent(
        QKeyEvent* event
    ) override;


    /**
     * @brief Обработка нажатия кнопок мыши.
     */
    void mousePressEvent(
        QMouseEvent* event
    ) override;


    /**
     * @brief Обработка отпускания кнопок мыши.
     */
    void mouseReleaseEvent(
        QMouseEvent* event
    ) override;


    /**
     * @brief Обработка движения мыши.
     */
    void mouseMoveEvent(
        QMouseEvent* event
    ) override;


    /**
     * @brief Обработка колеса мыши.
     */
    void wheelEvent(
        QWheelEvent* event
    ) override;


private:

    /*
     * Qt UI.
     */
    QLabel* title_label_ =
        nullptr;

    QLabel* content_label_ =
        nullptr;

    QLabel* coordinates_label_ =
        nullptr;


    /*
     * Renderer.
     */
    Renderer renderer_;


    /*
     * Camera.
     */
    Camera camera_;


    /*
     * Scene.
     */
    Scene scene_;


    /*
     * Текущий выбранный объект.
     */
    std::shared_ptr<SceneObject>
        selected_object_;


    /*
     * Callback изменения selection.
     */
    SelectionChangedCallback
        selection_changed_callback_;


    /*
     * Основной Shader сцены.
     */
    std::unique_ptr<Shader>
        shader_;


    /*
     * Editor Grid.
     */
    std::unique_ptr<Mesh>
        grid_mesh_;


    /*
     * Мировая ось X.
     */
    std::unique_ptr<Mesh>
        axis_x_mesh_;


    /*
     * Мировая ось Y.
     */
    std::unique_ptr<Mesh>
        axis_y_mesh_;


    /*
     * Мировая ось Z.
     */
    std::unique_ptr<Mesh>
        axis_z_mesh_;


    /*
     * Move Gizmo X.
     */
    std::unique_ptr<Mesh>
        gizmo_x_mesh_;


    /*
     * Move Gizmo Y.
     */
    std::unique_ptr<Mesh>
        gizmo_y_mesh_;


    /*
     * Move Gizmo Z.
     */
    std::unique_ptr<Mesh>
        gizmo_z_mesh_;


    /*
     * OBJ, ожидающий загрузки
     * после создания OpenGL Context.
     */
    std::optional<ImportedMeshData>
        pending_mesh_data_;


    /*
     * Путь к текущему импортированному файлу.
     */
    QString current_file_path_;


    /*
     * Цвет фона viewport.
     */
    Vec3 background_color_{
        0.12f,
        0.13f,
        0.14f
    };


    /*
     * Параметры, используемые
     * для первоначального отображения модели.
     */
    Vec3 model_position_{
        0.0f,
        0.0f,
        0.0f
    };

    Vec3 model_rotation_{
        0.0f,
        0.0f,
        0.0f
    };

    Vec3 model_scale_{
        1.0f,
        1.0f,
        1.0f
    };


    GizmoAxis active_gizmo_axis_ = GizmoAxis::None;

    /*
     * Состояние клавиш движения камеры.
     */
    bool move_forward_ =
        false;

    bool move_backward_ =
        false;

    bool move_left_ =
        false;

    bool move_right_ =
        false;

    bool move_up_ =
        false;

    bool move_down_ =
        false;


    /**
     * @brief Активно ли вращение камеры мышью.
     */
    bool pointer_look_active_ =
        false;


    /**
     * @brief Предыдущая позиция мыши
     * при вращении камеры.
     */
    QPointF last_pointer_position_;


    /**
     * @brief Таймер обработки движения камеры.
     */
    QTimer input_timer_;


    /**
     * @brief Таймер для вычисления delta time.
     */
    QElapsedTimer input_clock_;


    /**
     * @brief Текущий режим проекции камеры.
     */
    ProjectionMode projection_mode_ =
        ProjectionMode::Perspective;


    /**
     * @brief Половина вертикального размера
     * ортографической области просмотра.
     */
    float orthographic_half_height_ =
        5.0f;


    /**
     * @brief Инициализирован ли OpenGL Context.
     */
    bool gl_initialized_ =
        false;


    GizmoAxis PickMoveGizmoAxis(
        const QPointF& mouse_position
    ) const;

    bool TryBeginMoveGizmoDrag(
        const QPointF& mouse_position
    );

    bool gizmo_drag_active_ =
    false;
};