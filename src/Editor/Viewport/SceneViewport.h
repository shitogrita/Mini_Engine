#pragma once

#include "Engine/Assets/ImportedMeshData.h"
#include "Engine/Assets/TextureManager.h"
#include "Engine/Math/Ray.h"
#include "Engine/Math/matrix_types.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/PointShadowMap.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/PointLight.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneObject.h"
#include "Engine/Core/ExecutionMode.h"
#include "Engine/Scene/SceneUpdateSystem.h"
#include "Engine/Tests/TestScene.h"
#include "Engine/Diagnostics/PerformanceStats.h"

#include <chrono>
#include <optional>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QPointF>
#include <QString>
#include <QTimer>

#include <functional>
#include <memory>
#include <vector>

class QLabel;
class QFocusEvent;
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
 * - Move / Rotate / Scale Gizmo;
 * - дополнительный Point Shadow Pass.
 *
 * Обычное multi-light освещение и Shadow Pass разделены:
 * Depth Cubemap только дополняет текущий рендеринг и не заменяет его.
 */
class SceneViewport final : public QOpenGLWidget {
public:
    enum class ProjectionMode {
        Perspective,
        Orthographic
    };

    /**
     * @brief Тип SceneObject.
     *
     * Тип используется Editor для определения
     * специфического поведения объекта:
     * обычная геометрия, импортированная модель
     * или источник света.
     */
    enum class Type {
        Empty,
        Cube,
        Plane,
        Sphere,
        ImportedModel,
        PointLight
    };

    using SelectionChangedCallback = std::function<void(std::shared_ptr<SceneObject>)>;
    using TransformChangedCallback = std::function<void()>;
    using PerformanceStatsCallback = std::function<void(const PerformanceStats&)>;

    explicit SceneViewport(QWidget* parent = nullptr);
    ~SceneViewport() override;

    void SetDisplayedFile(const QString& file_path);

    void SetProjectionMode(ProjectionMode mode);
    ProjectionMode GetProjectionMode() const;

    Scene& GetScene();
    const Scene& GetScene() const;

    void SetSelectedObject(std::shared_ptr<SceneObject> object);
    std::shared_ptr<SceneObject> GetSelectedObject() const;

    void DeleteSelectedObject();
    void SetSelectionChangedCallback(SelectionChangedCallback callback);
    void SetTransformChangedCallback(TransformChangedCallback callback);

    void CreateCube();
    void CreatePlane();
    void CreateSphere();
    void ClearScene();

    void SetMaterialTexture(Material& material, const QString& file_path);
    bool LoadScene(const QString& file_path);

    /**
     * @brief Создаёт Point Light как отдельный объект сцены.
     *
     * Источник появляется в Hierarchy и может быть выбран
     * и перемещён так же, как остальные SceneObject.
     */
    void CreatePointLight();

    bool StartTest(TestScene test_scene, ExecutionMode execution_mode);
    void StopTest();
    bool IsTestRunning() const;

    void SetPerformanceStatsCallback(PerformanceStatsCallback callback);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    enum class GizmoAxis {
        None,
        X,
        Y,
        Z
    };

    enum class GizmoMode {
        Move,
        Rotate,
        Scale
    };

    void CreateLayout();
    void ImportPendingModel();
    void TickInput();
    void UpdateProjectionTitle();
    void UpdateCoordinatesLabel();
    void ArrangeSceneObjects();

    void CreateEditorGrid();
    void CreateMoveGizmo();
    void CreateRotateGizmo();

    ImportedMeshData CreateGridMeshData() const;
    ImportedMeshData CreateAxisMeshData(const Vec3& start, const Vec3& end) const;
    ImportedMeshData CreateCircleMeshData(GizmoAxis axis) const;

    Vec3 FindSpawnPosition() const;
    void ApplyModelFit(const std::vector<std::shared_ptr<SceneObject>>& objects, const Vec3& spawn_position);

    Ray CreateMouseRay(const QPointF& mouse_position) const;
    void SelectObjectAt(const QPointF& mouse_position);
    void NotifySelectionChanged();
    void FrameSelectedObject();

    void CreatePrimitive(const QString& name, ImportedMeshData mesh_data, SceneObject::Type type);
    void ResetInputState();

    GizmoAxis PickMoveScaleGizmoAxis(const QPointF& mouse_position) const;
    GizmoAxis PickRotateGizmoAxis(const QPointF& mouse_position) const;
    bool TryBeginGizmoDrag(const QPointF& mouse_position);

    float CalculateFrameDeltaTime(const std::chrono::steady_clock::time_point& frame_start);
    Matrix4 CreateProjectionMatrix() const;
    void PrepareMainShader(int shadow_light_index);
    void RenderGridAndAxes(const Matrix4& view_projection);
    void RenderSceneObjects(const Matrix4& view, const Matrix4& projection);
    void RenderPointLightMarkers(const Matrix4& view, const Matrix4& projection);
    void RenderTransformGizmo(const Matrix4& view, const Matrix4& projection);
    void UpdatePerformanceStats(const std::chrono::steady_clock::time_point& frame_start, const std::chrono::steady_clock::time_point& update_start, const std::chrono::steady_clock::time_point& update_end, const std::chrono::steady_clock::time_point& render_start, const std::chrono::steady_clock::time_point& frame_end);

    /**
     * @brief Строит Depth Cubemap для первого активного Point Light,
     * у которого включён Cast Shadows.
     *
     * Метод не заменяет обычный multi-light pass.
     *
     * @return Индекс shadow-casting источника в массиве uPointLights
     * или -1, если Shadow Pass не выполнялся.
     */
    int RenderPointShadowMap();

    QLabel* title_label_ = nullptr;
    QLabel* content_label_ = nullptr;
    QLabel* coordinates_label_ = nullptr;

    Renderer renderer_;
    Camera camera_;
    Scene scene_;

    std::shared_ptr<SceneObject> selected_object_;

    SelectionChangedCallback selection_changed_callback_;
    TransformChangedCallback transform_changed_callback_;

    std::unique_ptr<Shader> shader_;

    std::unique_ptr<Mesh> grid_mesh_;
    std::unique_ptr<Mesh> axis_x_mesh_;
    std::unique_ptr<Mesh> axis_y_mesh_;
    std::unique_ptr<Mesh> axis_z_mesh_;

    std::unique_ptr<Mesh> gizmo_x_mesh_;
    std::unique_ptr<Mesh> gizmo_y_mesh_;
    std::unique_ptr<Mesh> gizmo_z_mesh_;

    std::unique_ptr<Mesh> rotate_gizmo_x_mesh_;
    std::unique_ptr<Mesh> rotate_gizmo_y_mesh_;
    std::unique_ptr<Mesh> rotate_gizmo_z_mesh_;

    std::unique_ptr<Mesh> light_mesh_;
    std::unique_ptr<Shader> light_shader_;

    /**
     * @brief Shader depth-only прохода Point Shadow Mapping.
     *
     * Создаётся только если point_shadow.vert и point_shadow.frag
     * присутствуют в каталоге shaders.
     */
    std::unique_ptr<Shader> shadow_shader_;

    /**
     * @brief Depth Cubemap и framebuffer для Point Light shadows.
     *
     * Хранится через unique_ptr, чтобы OpenGL-ресурсы можно было
     * уничтожить явно, пока QOpenGLContext ещё активен.
     */
    std::unique_ptr<PointShadowMap> point_shadow_map_;

    QString pending_model_path_;
    TextureManager texture_manager_;
    QString current_file_path_;

    Vec3 background_color_{0.12f, 0.13f, 0.14f};

    GizmoAxis active_gizmo_axis_ = GizmoAxis::None;
    GizmoMode gizmo_mode_ = GizmoMode::Move;

    bool gizmo_drag_active_ = false;

    bool move_forward_ = false;
    bool move_backward_ = false;
    bool move_left_ = false;
    bool move_right_ = false;
    bool move_up_ = false;
    bool move_down_ = false;

    bool pointer_look_active_ = false;

    bool grid_visible_ = true;
    bool gizmo_visible_ = true;
    bool lighting_enabled_ = true;
    bool axes_visible_ = true;
    bool coordinates_visible_ = true;

    QPointF last_pointer_position_;

    QTimer input_timer_;
    QElapsedTimer input_clock_;

    ProjectionMode projection_mode_ = ProjectionMode::Perspective;
    float orthographic_half_height_ = 5.0f;
    bool gl_initialized_ = false;

    /**
     * @brief Создаёт editor-объект Point Light.
     *
     * @param select_object Если true, созданный источник
     * сразу становится выбранным.
     */
    void CreatePointLightObject(bool select_object);

    SceneUpdateSystem scene_update_system_;
    ExecutionMode execution_mode_ = ExecutionMode::SingleThreaded;

    std::chrono::steady_clock::time_point last_frame_time_{};
    bool first_frame_ = true;

    std::optional<Scene> scene_before_test_;
    bool test_running_ = false;

    PerformanceStatsCallback performance_stats_callback_;
    std::chrono::steady_clock::time_point previous_frame_time_{};


    double performance_elapsed_ms_ = 0.0;

    double performance_update_sum_ms_ = 0.0;
    double performance_render_sum_ms_ = 0.0;
    double performance_frame_sum_ms_ = 0.0;

    std::size_t performance_sample_count_ = 0;

    double benchmark_warmup_elapsed_ms_ = 0.0;
    double benchmark_measurement_elapsed_ms_ = 0.0;

    bool benchmark_measuring_ = false;
    bool benchmark_finished_ = false;

    static constexpr double kBenchmarkWarmupMs = 2000.0;
    static constexpr double kBenchmarkMeasurementMs = 5000.0;
};