#pragma once

#include "Engine/Assets/ImportedMeshData.h"
#include "Engine/Assets/TextureManager.h"
#include "Engine/Core/ExecutionMode.h"
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
#include "Engine/Scene/SceneUpdateSystem.h"
#include "Engine/Tests/TestScene.h"

#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QPointF>
#include <QString>
#include <QTimer>

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class QLabel;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

/**
 * @brief Итоговые показатели одного benchmark.
 */
struct PerformanceStats {
    std::size_t object_count = 0;
    std::size_t worker_count = 1;

    double update_ms = 0.0;
    double render_ms = 0.0;
    double frame_ms = 0.0;
    double fps = 0.0;

    double shadow_ms = 0.0;
    double main_ms = 0.0;
    double editor_ms = 0.0;

    std::size_t draw_calls = 0;
    std::size_t shadow_draw_calls = 0;
    std::size_t main_draw_calls = 0;
    std::size_t editor_draw_calls = 0;
};

/**
 * @brief OpenGL viewport редактора.
 *
 * SceneViewport отвечает за:
 * - отображение Scene;
 * - управление Camera;
 * - импорт моделей;
 * - выбор SceneObject;
 * - Editor Grid;
 * - Gizmo;
 * - Point Light;
 * - Point Shadow Mapping;
 * - SingleThreaded / MultiThreaded Scene Update;
 * - benchmark производительности;
 * - автоматический полный benchmark.
 */
class SceneViewport final : public QOpenGLWidget {
public:
    enum class ProjectionMode {
        Perspective,
        Orthographic
    };

    /**
     * @brief Тип Editor объекта.
     *
     * Сохранён для совместимости с существующей
     * архитектурой Editor.
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
    void CreatePointLight();

    void ClearScene();

    void SetMaterialTexture(Material& material, const QString& file_path);

    bool LoadScene(const QString& file_path);

    /**
     * @brief Запускает один benchmark.
     *
     * @param test_scene Размер тестовой Scene.
     * @param execution_mode SingleThreaded или MultiThreaded.
     * @param worker_count Количество workers для MultiThreaded.
     */
    bool StartTest(TestScene test_scene, ExecutionMode execution_mode, std::size_t worker_count);

    /**
     * @brief Останавливает текущий одиночный benchmark
     * и восстанавливает пользовательскую Scene.
     */
    void StopTest();

    bool IsTestRunning() const;

    void SetPerformanceStatsCallback(PerformanceStatsCallback callback);

    /**
     * @brief Запускает полный автоматический benchmark.
     *
     * Автоматически перебирает:
     *
     * Scene:
     * 1k
     * 10k
     * 25k
     * 50k
     *
     * Execution:
     * SingleThreaded
     * MultiThreaded
     *
     * Workers:
     * 1, 2, 4, 6, 8, hardware_concurrency().
     */
    bool StartBenchmarkSuite();

    /**
     * @brief Останавливает полный автоматический benchmark.
     */
    void StopBenchmarkSuite();

    bool IsBenchmarkSuiteRunning() const;

    /**
     * @brief Callback прогресса полного benchmark.
     *
     * Первый параметр — номер текущего теста.
     * Второй параметр — общее количество тестов.
     */
    void SetBenchmarkSuiteProgressCallback(std::function<void(std::size_t, std::size_t)> callback);

    /**
     * @brief Callback завершения полного benchmark.
     */
    void SetBenchmarkSuiteFinishedCallback(std::function<void()> callback);

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

    /**
     * @brief Одна конфигурация автоматического benchmark.
     *
     * Например:
     *
     * Cube25k
     * MultiThreaded
     * 4 workers
     */
    struct BenchmarkCase {
        std::size_t run_index;
        TestScene test_scene;
        ExecutionMode execution_mode;
        std::size_t worker_count;
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

    /**
     * @brief Создаёт editor SceneObject для Point Light.
     */
    void CreatePointLightObject(bool select_object);

    /**
     * @brief Строит Depth Cubemap для первого
     * активного Point Light с Cast Shadows.
     *
     * @return Индекс источника в uPointLights
     * или -1, если Shadow Pass не выполнялся.
     */
    int RenderPointShadowMap();

    /**
     * @brief Вычисляет delta time между кадрами.
     */
    float CalculateFrameDeltaTime(const std::chrono::steady_clock::time_point& frame_start);

    /**
     * @brief Создаёт текущую Projection Matrix.
     */
    Matrix4 CreateProjectionMatrix() const;

    /**
     * @brief Передаёт основные параметры Scene
     * в основной Shader.
     */
    void PrepareMainShader(int shadow_light_index);

    /**
     * @brief Рисует Editor Grid и мировые оси.
     */
    void RenderGridAndAxes(const Matrix4& view_projection);

    /**
     * @brief Рисует SceneObject.
     */
    void RenderSceneObjects(const Matrix4& view, const Matrix4& projection);

    /**
     * @brief Рисует визуальные маркеры Point Light.
     */
    void RenderPointLightMarkers(const Matrix4& view, const Matrix4& projection);

    /**
     * @brief Рисует Move / Rotate / Scale Gizmo.
     */
    void RenderTransformGizmo(const Matrix4& view, const Matrix4& projection);

    /**
     * @brief Обрабатывает время текущего benchmark.
     *
     * Выполняет:
     * - warm-up;
     * - накопление samples;
     * - вычисление средних значений;
     * - передачу результата в UI;
     * - CSV export;
     * - переход к следующему автоматическому тесту.
     */
    void UpdatePerformanceStats(const std::chrono::steady_clock::time_point& frame_start, const std::chrono::steady_clock::time_point& update_start, const std::chrono::steady_clock::time_point& update_end, const std::chrono::steady_clock::time_point& render_start, const std::chrono::steady_clock::time_point& frame_end);

    /**
     * @brief Запускает следующую конфигурацию
     * полного автоматического benchmark.
     */
    bool StartNextBenchmarkCase();

    /**
     * @brief Завершает текущую конфигурацию
     * и переключает benchmark suite на следующую.
     */
    void FinishCurrentBenchmarkCase();

private:
    QLabel* title_label_ = nullptr;
    QLabel* content_label_ = nullptr;
    QLabel* coordinates_label_ = nullptr;

    Renderer renderer_;
    Camera camera_;

    /**
     * @brief Текущая Scene.
     */
    Scene scene_;

    /**
     * @brief Пользовательская Scene временно хранится здесь
     * во время benchmark.
     */
    std::optional<Scene> scene_before_test_;

    std::shared_ptr<SceneObject> selected_object_;

    SelectionChangedCallback selection_changed_callback_;
    TransformChangedCallback transform_changed_callback_;
    PerformanceStatsCallback performance_stats_callback_;

    /**
     * @brief Основной Shader сцены.
     */
    std::unique_ptr<Shader> shader_;

    /**
     * @brief Editor Grid.
     */
    std::unique_ptr<Mesh> grid_mesh_;

    /**
     * @brief Мировые координатные оси.
     */
    std::unique_ptr<Mesh> axis_x_mesh_;
    std::unique_ptr<Mesh> axis_y_mesh_;
    std::unique_ptr<Mesh> axis_z_mesh_;

    /**
     * @brief Move / Scale Gizmo.
     */
    std::unique_ptr<Mesh> gizmo_x_mesh_;
    std::unique_ptr<Mesh> gizmo_y_mesh_;
    std::unique_ptr<Mesh> gizmo_z_mesh_;

    /**
     * @brief Rotate Gizmo.
     */
    std::unique_ptr<Mesh> rotate_gizmo_x_mesh_;
    std::unique_ptr<Mesh> rotate_gizmo_y_mesh_;
    std::unique_ptr<Mesh> rotate_gizmo_z_mesh_;

    /**
     * @brief Визуальное представление Point Light.
     */
    std::unique_ptr<Mesh> light_mesh_;
    std::unique_ptr<Shader> light_shader_;

    /**
     * @brief Shader Depth Pass для Point Shadow Mapping.
     */
    std::unique_ptr<Shader> shadow_shader_;

    /**
     * @brief Depth Cubemap и framebuffer
     * для Point Light shadows.
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
     * @brief Система CPU Update сцены.
     *
     * Внутри неё находится ThreadPool.
     */
    SceneUpdateSystem scene_update_system_;

    /**
     * @brief Текущий способ CPU Update.
     */
    ExecutionMode execution_mode_ = ExecutionMode::SingleThreaded;

    /**
     * @brief Выполняется ли сейчас обычный benchmark.
     */
    bool test_running_ = false;

    /**
     * @brief Первый кадр после запуска / остановки теста.
     */
    bool first_frame_ = true;

    /**
     * @brief Время предыдущего кадра.
     *
     * Используется для вычисления delta_time.
     */
    std::chrono::steady_clock::time_point previous_frame_time_{};

    /**
     * Старый аккумулятор оставляем для совместимости
     * с существующим кодом StopTest().
     *
     * Новый фиксированный benchmark использует
     * отдельные накопители ниже.
     */
    double performance_accumulator_ = 0.0;

    /**
     * @brief Фактически прошедшее время измерительного окна.
     */
    double performance_elapsed_ms_ = 0.0;

    /**
     * @brief Суммарные значения всех измеренных кадров.
     */
    double performance_update_sum_ms_ = 0.0;
    double performance_render_sum_ms_ = 0.0;
    double performance_frame_sum_ms_ = 0.0;
    double performance_shadow_sum_ms_ = 0.0;
    double performance_main_sum_ms_ = 0.0;
    double performance_editor_sum_ms_ = 0.0;

    std::size_t performance_draw_calls_sum_ = 0;
    std::size_t performance_shadow_draw_calls_sum_ = 0;
    std::size_t performance_main_draw_calls_sum_ = 0;
    std::size_t performance_editor_draw_calls_sum_ = 0;

    /**
     * @brief Количество кадров,
     * попавших в измерительное окно.
     */
    std::size_t performance_sample_count_ = 0;

    /**
     * @brief Сколько времени уже прошло
     * на этапе прогрева.
     */
    double benchmark_warmup_elapsed_ms_ = 0.0;

    /**
     * @brief Сколько времени уже прошло
     * в основном измерительном окне.
     */
    double benchmark_measurement_elapsed_ms_ = 0.0;

    std::chrono::steady_clock::time_point benchmark_warmup_start_time_{};
    std::chrono::steady_clock::time_point benchmark_measurement_start_time_{};

    /**
     * @brief true после завершения warm-up.
     */
    bool benchmark_measuring_ = false;

    /**
     * @brief true после получения окончательного результата.
     *
     * Пока флаг установлен,
     * новые кадры больше не меняют benchmark.
     */
    bool benchmark_finished_ = false;

    /**
     * @brief Продолжительность прогрева.
     */
    static constexpr double kBenchmarkWarmupMs = 2000.0;

    /**
     * @brief Продолжительность измерительного окна.
     */
    static constexpr double kBenchmarkMeasurementMs = 5000.0;

    static constexpr std::size_t kBenchmarkRepeatCount = 3;

    /**
     * @brief Список всех тестов автоматического benchmark.
     */
    std::vector<BenchmarkCase> benchmark_cases_;

    /**
     * @brief Индекс текущей конфигурации.
     */
    std::size_t benchmark_case_index_ = 0;

    /**
     * @brief true, пока выполняется Run Full Benchmark.
     */
    bool benchmark_suite_running_ = false;

    /**
     * @brief Сообщает UI текущий прогресс:
     *
     * current / total.
     */
    std::function<void(std::size_t, std::size_t)> benchmark_suite_progress_callback_;

    /**
     * @brief Вызывается после завершения
     * последнего автоматического теста.
     */
    std::function<void()> benchmark_suite_finished_callback_;
};