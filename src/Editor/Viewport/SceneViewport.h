#pragma once

#include "Engine/Assets/ImportedMeshData.h"
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

#include <memory>
#include <optional>


class QLabel;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;


class SceneViewport final : public QOpenGLWidget {
public:
    enum class ProjectionMode {
        Perspective,
        Orthographic
    };

public:
    explicit SceneViewport(
        QWidget* parent = nullptr
    );

    ~SceneViewport() override;

    void SetDisplayedFile(
        const QString& file_path
    );

    void SetProjectionMode(
        ProjectionMode mode
    );

    ProjectionMode GetProjectionMode() const;

    Scene& GetScene();
    const Scene& GetScene() const;

    void SetSelectedObject(
        std::shared_ptr<SceneObject> object
    );

private:
    void CreateLayout();

    void UploadPendingMesh();

    void CalculateModelFit(
        const ImportedMeshData& mesh_data
    );

    void TickInput();

    void UpdateProjectionTitle();

    void UpdateCoordinatesLabel();

    void ArrangeSceneObjects();

    void CreateEditorGrid();

    ImportedMeshData CreateGridMeshData() const;

    ImportedMeshData CreateAxisMeshData(
        const Vec3& start,
        const Vec3& end
    ) const;

    Vec3 FindSpawnPosition() const;

protected:
    void initializeGL() override;

    void resizeGL(
        int width,
        int height
    ) override;

    void paintGL() override;

    void keyPressEvent(
        QKeyEvent* event
    ) override;

    void keyReleaseEvent(
        QKeyEvent* event
    ) override;

    void mousePressEvent(
        QMouseEvent* event
    ) override;

    void mouseReleaseEvent(
        QMouseEvent* event
    ) override;

    void mouseMoveEvent(
        QMouseEvent* event
    ) override;

    void wheelEvent(
        QWheelEvent* event
    ) override;

private:
    QLabel* title_label_ = nullptr;
    QLabel* content_label_ = nullptr;
    QLabel* coordinates_label_ = nullptr;

    Renderer renderer_;

    Camera camera_;

    Scene scene_;

    std::shared_ptr<SceneObject>
        selected_object_;

    std::unique_ptr<Shader>
        shader_;

    std::unique_ptr<Mesh>
        grid_mesh_;

    std::unique_ptr<Mesh>
        axis_x_mesh_;

    std::unique_ptr<Mesh>
        axis_y_mesh_;

    std::unique_ptr<Mesh>
        axis_z_mesh_;

    std::optional<ImportedMeshData>
        pending_mesh_data_;

    QString current_file_path_;

    Vec3 background_color_{
        0.145f,
        0.153f,
        0.165f
    };

    Vec3 model_position_{};
    Vec3 model_rotation_{};

    Vec3 model_scale_{
        1.0f,
        1.0f,
        1.0f
    };

    bool move_forward_ = false;
    bool move_backward_ = false;
    bool move_left_ = false;
    bool move_right_ = false;
    bool move_up_ = false;
    bool move_down_ = false;

    bool pointer_look_active_ = false;

    QPointF last_pointer_position_;

    QTimer input_timer_;
    QElapsedTimer input_clock_;

    ProjectionMode projection_mode_ =
        ProjectionMode::Perspective;

    float orthographic_half_height_ =
        5.0f;

    bool gl_initialized_ = false;
};