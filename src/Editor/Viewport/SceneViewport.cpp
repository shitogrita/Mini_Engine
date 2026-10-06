#include "Editor/Viewport/SceneViewport.h"

#include "Engine/Assets/ModelImporter.h"
#include "Engine/Math/affine_transformation.h"
#include "Engine/Math/projection.h"
#include "Engine/Platform/OpenGL/OpenGLLoader.h"
#include "Engine/Scene/BoundingBox.h"
#include "Engine/Renderer/PrimitiveGenerator.h"
#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Tests/Cube1kTest.h"

#include <QByteArray>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QWheelEvent>
#include <QFocusEvent>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

/**
 * @brief Вертикальный угол обзора Perspective-камеры.
 *
 * Значение должно совпадать с FOV,
 * используемым при построении Projection Matrix.
 */
static constexpr float kPerspectiveFovDegrees = 45.0f;

/**
 * @brief Ближняя плоскость отсечения.
 */
static constexpr float kNearPlane = 0.1f;

/**
 * @brief Дальняя плоскость отсечения.
 */
static constexpr float kFarPlane = 100.0f;

/**
 * @brief Число PI для перевода градусов в радианы.
 */
static constexpr float kPi = 3.14159265358979323846f;

/**
 * @brief Минимальное значение,
 * используемое при проверках float.
 */
static constexpr float kVectorEpsilon = 0.000001f;

/**
 * @brief Максимальное число Point Light, одновременно передаваемых в basic.frag.
 *
 * Ограничение фиксировано на стороне GLSL 3.30 и позволяет не использовать
 * более сложные SSBO/UBO-механизмы до этапа дальнейшего развития Renderer.
 */
static constexpr int kMaxPointLights = 8;

/**
 * @brief Разрешение одной стороны Point Shadow Cubemap.
 */
static constexpr int kPointShadowResolution = 1024;

/**
 * @brief Ближняя плоскость shadow projection.
 */
static constexpr float kPointShadowNearPlane = 0.1f;

/**
 * @brief Дальняя плоскость shadow projection.
 *
 * Значение также используется Fragment Shader для восстановления
 * реального расстояния, записанного в Depth Cubemap.
 */
static constexpr float kPointShadowFarPlane = 50.0f;

/**
 * @brief Возвращает OpenGL-функцию
 * из текущего Qt OpenGL Context.
 *
 * Используется для инициализации GLAD
 * через текущий QOpenGLContext.
 */
static void* GetQtOpenGLProcAddress(const char* name) {
    QOpenGLContext* current_context =
        QOpenGLContext::currentContext();

    if (current_context == nullptr) {
        return nullptr;
    }

    const QFunctionPointer function =
        current_context->getProcAddress(QByteArray(name));

    return reinterpret_cast<void*>(function);
}

/**
 * @brief Возвращает длину Vec3.
 */
static float Length(const Vec3& vector) {
    return std::sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

/**
 * @brief Нормализует Vec3.
 *
 * Если длина практически равна нулю,
 * возвращается нулевой вектор.
 */
static Vec3 Normalize(const Vec3& vector) {
    const float length =
        Length(vector);

    if (length < kVectorEpsilon) {
        return Vec3{};
    }

    return Vec3{
        vector.x / length,
        vector.y / length,
        vector.z / length
    };
}

/**
 * @brief Возвращает скалярное произведение двух Vec3.
 */
static float Dot(const Vec3& first, const Vec3& second) {
    return first.x * second.x + first.y * second.y + first.z * second.z;
}

/**
 * @brief Возвращает векторное произведение двух Vec3.
 */
static Vec3 Cross(const Vec3& first, const Vec3& second) {
    return Vec3{
        first.y * second.z - first.z * second.y,
        first.z * second.x - first.x * second.z,
        first.x * second.y - first.y * second.x
    };
}

/**
 * @brief Создаёт View Matrix для одной стороны Point Shadow Cubemap.
 *
 * Матрица использует ту же row-major структуру Matrix4,
 * что и остальные матрицы Mini Engine. Перед отправкой в OpenGL
 * Shader::SetMat4() преобразует её в column-major representation.
 *
 * @param eye Позиция Point Light.
 * @param direction Направление взгляда стороны Cubemap.
 * @param up Направление "вверх" для стороны Cubemap.
 * @return View Matrix источника света.
 */
static Matrix4 CreateLookAtMatrix(const Vec3& eye, const Vec3& direction, const Vec3& up) {
    const Vec3 forward = Normalize(direction);
    const Vec3 right = Normalize(Cross(forward, up));
    const Vec3 corrected_up = Cross(right, forward);

    Matrix4 view{};

    view[0][0] = right.x;
    view[0][1] = right.y;
    view[0][2] = right.z;
    view[0][3] = -Dot(right, eye);

    view[1][0] = corrected_up.x;
    view[1][1] = corrected_up.y;
    view[1][2] = corrected_up.z;
    view[1][3] = -Dot(corrected_up, eye);

    view[2][0] = -forward.x;
    view[2][1] = -forward.y;
    view[2][2] = -forward.z;
    view[2][3] = Dot(forward, eye);

    view[3][0] = 0.0f;
    view[3][1] = 0.0f;
    view[3][2] = 0.0f;
    view[3][3] = 1.0f;

    return view;
}

/**
 * @brief Проверяет пересечение World Space Ray со сферой.
 *
 * Используется для выбора Point Light мышью.
 *
 * Визуальная сфера источника не является обычным Mesh
 * SceneObject, поэтому для неё используется отдельный
 * простой sphere picking.
 *
 * @param ray Луч из Camera через положение курсора.
 * @param center Центр сферы в World Space.
 * @param radius Радиус области выбора.
 * @param distance Расстояние до пересечения.
 *
 * @return true, если Ray пересекает сферу.
 */
static bool IntersectRayWithSphere(const Ray& ray, const Vec3& center, float radius, float& distance) {
    const Vec3 offset{
        ray.origin.x - center.x,
        ray.origin.y - center.y,
        ray.origin.z - center.z
    };

    const float b =
        offset.x * ray.direction.x +
        offset.y * ray.direction.y +
        offset.z * ray.direction.z;

    const float c =
        offset.x * offset.x +
        offset.y * offset.y +
        offset.z * offset.z -
        radius * radius;

    const float discriminant =
        b * b - c;

    if (discriminant < 0.0f) {
        return false;
    }

    const float root =
        std::sqrt(discriminant);

    float result =
        -b - root;

    if (result < 0.0f) {
        result =
            -b + root;
    }

    if (result < 0.0f) {
        return false;
    }

    distance = result;

    return true;
}

/**
 * @brief Возвращает расстояние между двумя точками.
 */
static float Distance(const Vec3& first, const Vec3& second) {
    const Vec3 difference{
        first.x - second.x,
        first.y - second.y,
        first.z - second.z
    };

    return Length(difference);
}

/**
 * @brief Возвращает минимальное расстояние между Ray и отрезком.
 *
 * Используется для выбора осей и колец Gizmo мышью.
 */
static float DistanceRayToSegment(const Ray& ray, const Vec3& start, const Vec3& end) {
    const Vec3 segment{
        end.x - start.x,
        end.y - start.y,
        end.z - start.z
    };

    const Vec3 from_ray_to_segment{
        ray.origin.x - start.x,
        ray.origin.y - start.y,
        ray.origin.z - start.z
    };

    const float a = ray.direction.x * ray.direction.x + ray.direction.y * ray.direction.y + ray.direction.z * ray.direction.z;
    const float b = ray.direction.x * segment.x + ray.direction.y * segment.y + ray.direction.z * segment.z;
    const float c = segment.x * segment.x + segment.y * segment.y + segment.z * segment.z;
    const float d = ray.direction.x * from_ray_to_segment.x + ray.direction.y * from_ray_to_segment.y + ray.direction.z * from_ray_to_segment.z;
    const float e = segment.x * from_ray_to_segment.x + segment.y * from_ray_to_segment.y + segment.z * from_ray_to_segment.z;
    const float denominator = a * c - b * b;

    float ray_parameter = 0.0f;
    float segment_parameter = 0.0f;

    if (std::abs(denominator) > kVectorEpsilon) {
        ray_parameter = (b * e - c * d) / denominator;
        segment_parameter = (a * e - b * d) / denominator;
    }

    ray_parameter = std::max(ray_parameter, 0.0f);
    segment_parameter = std::clamp(segment_parameter, 0.0f, 1.0f);

    const Vec3 point_on_ray{
        ray.origin.x + ray.direction.x * ray_parameter,
        ray.origin.y + ray.direction.y * ray_parameter,
        ray.origin.z + ray.direction.z * ray_parameter
    };

    const Vec3 point_on_segment{
        start.x + segment.x * segment_parameter,
        start.y + segment.y * segment_parameter,
        start.z + segment.z * segment_parameter
    };

    return Distance(point_on_ray, point_on_segment);
}

/**
 * @brief Создаёт SceneViewport и запускает обработку управления Camera.
 *
 * Viewport получает клавиатурный focus для WASD/QE.
 *
 * input_timer_ примерно 60 раз в секунду вызывает TickInput(),
 * который преобразует состояние клавиш в движение Camera.
 *
 * Также при создании сцены добавляется стандартный Point Light
 * как обычный SceneObject.
 *
 * @param parent Родительский QWidget.
 */
SceneViewport::SceneViewport(QWidget* parent) : QOpenGLWidget(parent) {
    /*
     * Viewport должен принимать клавиатурный focus,
     * иначе keyPressEvent/keyReleaseEvent не будут
     * получать WASD после клика по Scene.
     */
    setFocusPolicy(Qt::StrongFocus);

    /*
     * Позволяет получать движение мыши
     * для управления Camera и Gizmo.
     */
    setMouseTracking(true);

    CreateLayout();

    /*
     * Создаём стандартный Point Light как SceneObject.
     *
     * false означает, что при запуске Editor
     * источник света не выбирается автоматически.
     */
    CreatePointLightObject(false);

    /*
     * QElapsedTimer используется для вычисления delta time
     * между обновлениями Camera.
     */
    input_clock_.start();

    /*
     * input_timer_ является главным циклом
     * клавиатурного движения Camera.
     *
     * Без этого таймера W/A/S/D только меняют bool-флаги,
     * но сама Camera никогда не получает MoveForward/MoveLeft и т.д.
     */
    input_timer_.setTimerType(Qt::PreciseTimer);

    connect(&input_timer_, &QTimer::timeout, this, [this]() {
        TickInput();
        UpdateCoordinatesLabel();
        update();
    });

    /*
     * Около 60 обновлений в секунду.
     */
    input_timer_.start(16);
}

SceneViewport::~SceneViewport() {
    /*
     * OpenGL-ресурсы должны уничтожаться,
     * пока соответствующий Context активен.
     */
    if (context() != nullptr) {
        makeCurrent();

        scene_.Clear();
        texture_manager_.Clear();

        /*
         * Editor Grid и мировые оси.
         */
        grid_mesh_.reset();

        axis_x_mesh_.reset();
        axis_y_mesh_.reset();
        axis_z_mesh_.reset();

        /*
         * Move Gizmo.
         *
         * Mesh также содержит OpenGL-ресурсы,
         * поэтому уничтожаем его до doneCurrent().
         */
        gizmo_x_mesh_.reset();
        gizmo_y_mesh_.reset();
        gizmo_z_mesh_.reset();

        rotate_gizmo_x_mesh_.reset();
        rotate_gizmo_y_mesh_.reset();
        rotate_gizmo_z_mesh_.reset();

        /*
         * Point Shadow Map содержит OpenGL framebuffer
         * и Depth Cubemap, поэтому освобождаем его,
         * пока Context ещё активен.
         */
        point_shadow_map_.reset();
        shadow_shader_.reset();

        shader_.reset();
        light_mesh_.reset();
        light_shader_.reset();

        doneCurrent();
    }
}

void SceneViewport::CreateLayout() {
    setMinimumSize(500, 400);

    /*
     * Верхняя строка Scene View.
     */
    title_label_ =
        new QLabel("Scene", this);

    title_label_->setFixedHeight(34);

    title_label_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    title_label_->setContentsMargins(12, 0, 0, 0);

    title_label_->setStyleSheet(R"(background-color: #3c3f41; color: #d7dae0; border-bottom: 1px solid #51555a; font-size: 13px; font-weight: 600;)"
    );

    /*
     * Сообщение, показываемое,
     * пока ни одной модели нет.
     */
    content_label_ =
        new QLabel(this);

    content_label_->setAlignment(Qt::AlignCenter);

    content_label_->setText("Scene View\n\n" "Open a model using\n" "File → Open Model...");

    content_label_->setStyleSheet(R"(background-color: #25282b; color: #a9adb3; font-size: 16px;)"
    );

    /*
     * Панель информации о Camera
     * и выбранном SceneObject.
     */
    coordinates_label_ =
        new QLabel(this);

    coordinates_label_->setAlignment(Qt::AlignRight | Qt::AlignTop);

    coordinates_label_->setStyleSheet(
        R"(
            background-color: rgba(48, 51, 55, 210);
            color: #c5c8ce;
            border: 1px solid #55595f;
            border-radius: 4px;
            padding: 7px 9px;
            font-family: Menlo;
            font-size: 11px;
        )"
    );

    coordinates_label_->setText("Camera\n" "X 0.000\n" "Y 0.000\n" "Z 3.000");

    coordinates_label_->adjustSize();

    UpdateProjectionTitle();
}

void SceneViewport::initializeGL() {
    /*
     * Загружаем OpenGL-функции
     * через текущий Qt OpenGL Context.
     */
    if (!InitializeOpenGLLoader(&GetQtOpenGLProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD for Qt OpenGL context");
    }

    renderer_.Initialize();

    /*
     * Загружаем основной Shader.
     */
    const std::filesystem::path shader_directory =
        MINI_ENGINE_SHADER_DIR;

    shader_ =
        std::make_unique<Shader>(shader_directory / "basic.vert", shader_directory / "basic.frag");
    light_shader_ = std::make_unique<Shader>(std::filesystem::path(MINI_ENGINE_SHADER_DIR) / "lamp.vert", std::filesystem::path(MINI_ENGINE_SHADER_DIR) / "lamp.frag");

    light_mesh_ = std::make_unique<Mesh>(PrimitiveGenerator::CreateSphere(0.08f, 16, 8));

    /**
     * @brief Создание ресурсов Point Shadow Mapping.
     *
     * Shadow shaders являются обязательной частью текущего Renderer.
     * Если один из файлов отсутствует, ошибка должна быть видна сразу,
     * а не приводить к тихому отключению теней.
     */
    const std::filesystem::path shadow_vertex_path =
        shader_directory / "pointShadow.vert";

    const std::filesystem::path shadow_fragment_path =
        shader_directory / "pointShadow.frag";

    if (!std::filesystem::exists(shadow_vertex_path)) {
        throw std::runtime_error(
            "Point shadow vertex shader not found: " +
            shadow_vertex_path.string()
        );
    }

    if (!std::filesystem::exists(shadow_fragment_path)) {
        throw std::runtime_error(
            "Point shadow fragment shader not found: " +
            shadow_fragment_path.string()
        );
    }

    shadow_shader_ =
        std::make_unique<Shader>(
            shadow_vertex_path,
            shadow_fragment_path
        );

    point_shadow_map_ =
        std::make_unique<PointShadowMap>();

    if (!point_shadow_map_->Initialize(kPointShadowResolution)) {
        throw std::runtime_error(
            "Failed to initialize Point Shadow Map"
        );
    }

    /*
     * Editor helpers:
     *
     * - Grid;
     * - X axis;
     * - Y axis;
     * - Z axis.
     */
    CreateEditorGrid();

    /*
     * Transform helper выбранного объекта.
     */
    CreateMoveGizmo();
    CreateRotateGizmo();

    gl_initialized_ = true;

    /*
     * Если OBJ был открыт ещё до создания
     * OpenGL Context, загружаем его сейчас.
     */
    ImportPendingModel();
}

void SceneViewport::CreateEditorGrid() {
    grid_mesh_ =
        std::make_unique<Mesh>(CreateGridMeshData());

    axis_x_mesh_ =
        std::make_unique<Mesh>(
            CreateAxisMeshData(
                Vec3{
                    -10.0f,
                    0.0f,
                    0.0f
                },
                Vec3{
                    10.0f,
                    0.0f,
                    0.0f
                }
            )
        );

    axis_y_mesh_ =
        std::make_unique<Mesh>(
            CreateAxisMeshData(
                Vec3{
                    0.0f,
                    -10.0f,
                    0.0f
                },
                Vec3{
                    0.0f,
                    10.0f,
                    0.0f
                }
            )
        );

    axis_z_mesh_ =
        std::make_unique<Mesh>(
            CreateAxisMeshData(
                Vec3{
                    0.0f,
                    0.0f,
                    -10.0f
                },
                Vec3{
                    0.0f,
                    0.0f,
                    10.0f
                }
            )
        );
}

ImportedMeshData SceneViewport::CreateGridMeshData() const {
    ImportedMeshData data;

    constexpr int half_grid = 10;

    constexpr float step = 1.0f;

    /*
     * Строим Grid на плоскости XZ.
     *
     * Центральные линии не рисуем,
     * потому что вместо них отдельно
     * будут нарисованы оси X и Z.
     */
    for (int index = -half_grid; index <= half_grid; ++index) {
        if (index == 0) {
            continue;
        }

        const float coordinate =
            static_cast<float>(index) *
            step;

        /*
         * Линия, параллельная Z.
         */
        Vertex first_z{};

        first_z.position =
            Vec3{
                coordinate,
                0.0f,
                -half_grid * step
            };

        Vertex second_z{};

        second_z.position =
            Vec3{
                coordinate,
                0.0f,
                half_grid * step
            };

        const std::uint32_t first_index =
            static_cast<std::uint32_t>(data.render_vertices.size());

        data.render_vertices.push_back(first_z);

        data.render_vertices.push_back(second_z);

        data.render_indices.push_back(first_index);

        data.render_indices.push_back(first_index + 1);

        /*
         * Линия, параллельная X.
         */
        Vertex first_x{};

        first_x.position =
            Vec3{
                -half_grid * step,
                0.0f,
                coordinate
            };

        Vertex second_x{};

        second_x.position =
            Vec3{
                half_grid * step,
                0.0f,
                coordinate
            };

        const std::uint32_t second_index =
            static_cast<std::uint32_t>(data.render_vertices.size());

        data.render_vertices.push_back(first_x);

        data.render_vertices.push_back(second_x);

        data.render_indices.push_back(second_index);

        data.render_indices.push_back(second_index + 1);
    }

    return data;
}

void SceneViewport::CreateMoveGizmo() {
    /*
     * Gizmo создаётся около локального начала координат.
     *
     * Позже при рендере мы будем переносить его
     * в world position выбранного SceneObject.
     */
    gizmo_x_mesh_ =
        std::make_unique<Mesh>(
            CreateAxisMeshData(
                Vec3{
                    0.0f,
                    0.0f,
                    0.0f
                },
                Vec3{
                    1.0f,
                    0.0f,
                    0.0f
                }
            )
        );

    gizmo_y_mesh_ =
        std::make_unique<Mesh>(
            CreateAxisMeshData(
                Vec3{
                    0.0f,
                    0.0f,
                    0.0f
                },
                Vec3{
                    0.0f,
                    1.0f,
                    0.0f
                }
            )
        );

    gizmo_z_mesh_ =
        std::make_unique<Mesh>(
            CreateAxisMeshData(
                Vec3{
                    0.0f,
                    0.0f,
                    0.0f
                },
                Vec3{
                    0.0f,
                    0.0f,
                    1.0f
                }
            )
        );
}

void SceneViewport::CreateRotateGizmo() {
    rotate_gizmo_x_mesh_ = std::make_unique<Mesh>(CreateCircleMeshData(GizmoAxis::X));
    rotate_gizmo_y_mesh_ = std::make_unique<Mesh>(CreateCircleMeshData(GizmoAxis::Y));
    rotate_gizmo_z_mesh_ = std::make_unique<Mesh>(CreateCircleMeshData(GizmoAxis::Z));
}

ImportedMeshData SceneViewport::CreateAxisMeshData(const Vec3& start, const Vec3& end) const {
    ImportedMeshData data;

    Vertex first{};

    first.position =
        start;

    Vertex second{};

    second.position =
        end;

    data.render_vertices.push_back(first);

    data.render_vertices.push_back(second);

    data.render_indices.push_back(0);

    data.render_indices.push_back(1);

    return data;
}

ImportedMeshData SceneViewport::CreateCircleMeshData(GizmoAxis axis) const {
    ImportedMeshData data;

    constexpr int segment_count = 64;
    constexpr float radius = 1.0f;

    for (int index = 0; index < segment_count; ++index) {
        const float first_angle = 2.0f * kPi * static_cast<float>(index) / static_cast<float>(segment_count);
        const float second_angle = 2.0f * kPi * static_cast<float>(index + 1) / static_cast<float>(segment_count);

        Vec3 first{};
        Vec3 second{};

        if (axis == GizmoAxis::X) {
            first = Vec3{0.0f, std::cos(first_angle) * radius, std::sin(first_angle) * radius};
            second = Vec3{0.0f, std::cos(second_angle) * radius, std::sin(second_angle) * radius};
        }

        if (axis == GizmoAxis::Y) {
            first = Vec3{std::cos(first_angle) * radius, 0.0f, std::sin(first_angle) * radius};
            second = Vec3{std::cos(second_angle) * radius, 0.0f, std::sin(second_angle) * radius};
        }

        if (axis == GizmoAxis::Z) {
            first = Vec3{std::cos(first_angle) * radius, std::sin(first_angle) * radius, 0.0f};
            second = Vec3{std::cos(second_angle) * radius, std::sin(second_angle) * radius, 0.0f};
        }

        const std::uint32_t first_index = static_cast<std::uint32_t>(data.render_vertices.size());

        Vertex first_vertex{};
        first_vertex.position = first;

        Vertex second_vertex{};
        second_vertex.position = second;

        data.render_vertices.push_back(first_vertex);
        data.render_vertices.push_back(second_vertex);

        data.render_indices.push_back(first_index);
        data.render_indices.push_back(first_index + 1);
    }

    return data;
}

/**
 * @brief Строит Depth Cubemap для первого активного Point Light с Cast Shadows.
 *
 * Обычное multi-light освещение при этом не изменяется:
 * Shadow Pass только создаёт дополнительную depth texture.
 *
 * В текущем этапе один PointShadowMap используется для одного
 * shadow-casting источника за кадр. Остальные Point Light
 * продолжают участвовать в обычном освещении.
 *
 * @return Индекс shadow-casting источника в uPointLights
 * или -1, если Shadow Pass в этом кадре не выполнялся.
 */
int SceneViewport::RenderPointShadowMap() {
    if (!lighting_enabled_ || !shadow_shader_ || !point_shadow_map_ || !point_shadow_map_->IsInitialized()) {
        return -1;
    }

    const SceneObject* shadow_light_object = nullptr;
    int shadow_light_index = -1;
    int active_light_index = 0;

    /*
     * Порядок должен полностью совпадать с порядком,
     * которым paintGL() заполняет uPointLights.
     */
    for (const std::shared_ptr<SceneObject>& object : scene_.GetObjects()) {
        if (!object || !object->HasPointLight()) {
            continue;
        }

        const PointLight* light = object->GetPointLight();

        if (!light || !light->IsEnabled()) {
            continue;
        }

        if (active_light_index >= kMaxPointLights) {
            break;
        }

        if (light->CastsShadows()) {
            shadow_light_object = object.get();
            shadow_light_index = active_light_index;
            break;
        }

        ++active_light_index;
    }

    if (shadow_light_object == nullptr) {
        return -1;
    }

    const Vec3 light_position = shadow_light_object->GetTransform().position;

    /*
     * Point Light имеет угол обзора 90 градусов для каждой
     * из шести граней Cubemap.
     */
    const Matrix4 shadow_projection = Projection::Perspective(
        90.0f,
        1.0f,
        kPointShadowNearPlane,
        kPointShadowFarPlane
    );

    const Vec3 directions[6] = {
        Vec3{ 1.0f,  0.0f,  0.0f},
        Vec3{-1.0f,  0.0f,  0.0f},
        Vec3{ 0.0f,  1.0f,  0.0f},
        Vec3{ 0.0f, -1.0f,  0.0f},
        Vec3{ 0.0f,  0.0f,  1.0f},
        Vec3{ 0.0f,  0.0f, -1.0f}
    };

    /*
     * Up-векторы выбраны специально для Cubemap,
     * чтобы ориентация соседних граней совпадала.
     */
    const Vec3 up_vectors[6] = {
        Vec3{0.0f, -1.0f,  0.0f},
        Vec3{0.0f, -1.0f,  0.0f},
        Vec3{0.0f,  0.0f,  1.0f},
        Vec3{0.0f,  0.0f, -1.0f},
        Vec3{0.0f, -1.0f,  0.0f},
        Vec3{0.0f, -1.0f,  0.0f}
    };

    shadow_shader_->Use();
    shadow_shader_->SetVec3("uLightPosition", light_position);
    shadow_shader_->SetFloat("uFarPlane", kPointShadowFarPlane);

    /*
     * Рендерим Scene шесть раз — по одному разу
     * для каждой стороны Depth Cubemap.
     */
    for (int face_index = 0; face_index < 6; ++face_index) {
        point_shadow_map_->BeginFace(face_index);

        const Matrix4 shadow_view = CreateLookAtMatrix(
            light_position,
            directions[face_index],
            up_vectors[face_index]
        );

        const Matrix4 light_view_projection = AffineTransformation::Multiply4(
            shadow_projection,
            shadow_view
        );

        shadow_shader_->SetMat4("uLightVP", light_view_projection);

        for (const std::shared_ptr<SceneObject>& object : scene_.GetObjects()) {
            if (!object || !object->HasMesh()) {
                continue;
            }

            const Matrix4 model = object->GetTransform().GetModelMatrix();
            shadow_shader_->SetMat4("uModel", model);

            /*
             * ImportedModel может состоять из нескольких Mesh.
             * Все части должны участвовать в построении тени.
             */
            if (object->HasRenderParts()) {
                for (const SceneObject::SceneRenderPart& part : object->GetRenderParts()) {
                    if (part.mesh) {
                        renderer_.DrawDepth(*part.mesh, *shadow_shader_);
                    }
                }

                continue;
            }

            const std::shared_ptr<const Mesh> mesh = object->GetMesh();

            if (mesh) {
                renderer_.DrawDepth(*mesh, *shadow_shader_);
            }
        }
    }
    /*
     * QOpenGLWidget использует собственный framebuffer.
     * После Shadow Pass возвращаем именно его,
     * а не предполагаемый framebuffer 0.
     */
    renderer_.BindFramebuffer(
        static_cast<unsigned int>(
            defaultFramebufferObject()
        )
    );

    /**
     * @brief Восстанавливаем размер viewport после Shadow Pass.
     *
     * QOpenGLWidget работает с High DPI.
     * width() и height() возвращают логический размер Widget,
     * а OpenGL framebuffer использует физические пиксели.
     *
     * На Retina devicePixelRatioF() обычно равен 2.0.
     */
    const float pixel_ratio = static_cast<float>(devicePixelRatioF());

    renderer_.SetViewport(
        static_cast<int>(static_cast<float>(width()) * pixel_ratio),
        static_cast<int>(static_cast<float>(height()) * pixel_ratio)
    );
    return shadow_light_index;
}

void SceneViewport::resizeGL(int width, int height) {
    renderer_.SetViewport(width, height);

    if (title_label_ != nullptr) {
        title_label_->setGeometry(0, 0, width, 34);
    }

    if (content_label_ != nullptr) {
        content_label_->setGeometry(0, 34, width, std::max(0, height - 34));
    }

    if (coordinates_label_ != nullptr) {
        coordinates_label_->adjustSize();

        coordinates_label_->move(std::max(10, width - coordinates_label_->width() - 14), 48);
    }
}

void SceneViewport::paintGL() {
    if (!shader_) {
        return;
    }

    const auto current_time = std::chrono::steady_clock::now();

    float delta_time = 0.0f;

    if (!first_frame_) {
        delta_time = std::chrono::duration<float>(current_time - last_frame_time_).count();
    }

    last_frame_time_ = current_time;
    first_frame_ = false;

    scene_update_system_.Update(scene_, delta_time, execution_mode_);

    /*
     * Shadow Pass выполняется ДО обычного Color Pass.
     *
     * RenderPointShadowMap() после завершения восстанавливает
     * framebuffer QOpenGLWidget и обычный viewport.
     */
    const int shadow_light_index = RenderPointShadowMap();

    renderer_.BeginFrame(background_color_);

    const Matrix4 view = camera_.GetViewMatrix();

    const float aspect = height() > 0
        ? static_cast<float>(width()) / static_cast<float>(height())
        : 1.0f;

    Matrix4 projection{};

    if (projection_mode_ == ProjectionMode::Perspective) {
        projection = Projection::Perspective(kPerspectiveFovDegrees, aspect, kNearPlane, kFarPlane);
    } else {
        const float half_height = orthographic_half_height_;
        const float half_width = half_height * aspect;

        projection = Projection::Ortho(-half_width, half_width, -half_height, half_height, kNearPlane, kFarPlane);
    }

    const Matrix4 view_projection = AffineTransformation::Multiply4(projection, view);

    shader_->Use();

    shader_->SetInt("uPointMode", 0);
    shader_->SetFloat("uPointSize", 6.0f);
    shader_->SetFloat("uPointSoft", 0.05f);
    shader_->SetFloat("uDashFill", 1.0f);

    /*
     * Передаём в basic.frag все включённые Point Light.
     *
     * Каждый источник хранится внутри собственного SceneObject:
     * - Position берётся из Transform;
     * - Color / Intensity / Enabled берутся из PointLight.
     *
     * basic.frag поддерживает до kMaxPointLights источников одновременно.
     * Лишние источники остаются объектами Scene, но в текущем кадре
     * не участвуют в расчёте освещения.
     */
    int point_light_count = 0;

    for (const std::shared_ptr<SceneObject>& object : scene_.GetObjects()) {
        if (!object || !object->HasPointLight()) {
            continue;
        }

        const PointLight* light = object->GetPointLight();

        if (!light || !light->IsEnabled()) {
            continue;
        }

        if (point_light_count >= kMaxPointLights) {
            break;
        }

        const std::string uniform_prefix =
            "uPointLights[" + std::to_string(point_light_count) + "]";

        shader_->SetVec3(
            uniform_prefix + ".position",
            object->GetTransform().position
        );

        shader_->SetVec3(
            uniform_prefix + ".color",
            light->GetColor()
        );

        shader_->SetFloat(
            uniform_prefix + ".intensity",
            light->GetIntensity()
        );

        ++point_light_count;
    }

    /*
     * Shader обрабатывает только реально переданные источники.
     * Disabled Point Light в этот счётчик не попадает.
     */
    shader_->SetInt(
        "uPointLightCount",
        point_light_count
    );

    shader_->SetVec3(
        "uViewPosition",
        camera_.GetPosition()
    );

    /*
     * Shadow uniforms являются дополнением к обычным Point Light.
     *
     * Если basic.frag ещё не содержит эти uniforms,
     * OpenGL вернёт location = -1 и текущий renderer
     * продолжит работать как раньше.
     */
    shader_->SetInt("uShadowLightIndex", shadow_light_index);
    shader_->SetFloat("uShadowFarPlane", kPointShadowFarPlane);
    shader_->SetInt("uPointShadowMap", 1);

    if (shadow_light_index >= 0 && point_shadow_map_) {
        point_shadow_map_->BindTexture(1);
    }

    /*
     * Grid и мировые оси не используют освещение
     * и не должны использовать текстуру предыдущего объекта.
     */
    shader_->SetInt("uLightingEnabled",  0);
    shader_->SetInt("uHasDiffuseTexture", 0);

    if (grid_visible_ && grid_mesh_) {
        shader_->SetVec4("uColor", Vec4{0.30f, 0.32f, 0.35f, 1.0f});
        renderer_.DrawLines(*grid_mesh_, *shader_, view_projection, 1.0f);
    }

    if (axes_visible_ && axis_x_mesh_) {
        shader_->SetVec4("uColor", Vec4{0.82f, 0.30f, 0.30f, 1.0f});
        renderer_.DrawLines(*axis_x_mesh_, *shader_, view_projection, 2.0f);
    }

    if (axes_visible_ && axis_z_mesh_) {
        shader_->SetVec4("uColor", Vec4{0.30f, 0.52f, 0.90f, 1.0f});
        renderer_.DrawLines(*axis_z_mesh_, *shader_, view_projection, 2.0f);
    }

    if (axes_visible_ && axis_y_mesh_) {
        shader_->SetVec4("uColor", Vec4{0.40f, 0.75f, 0.42f, 1.0f});
        renderer_.DrawLines(*axis_y_mesh_, *shader_, view_projection, 2.0f);
    }

    /*
     * Всё, что находится в Scene, рисуется
     * с освещением.
     */
    shader_->SetInt("uLightingEnabled", lighting_enabled_ ? 1 : 0);

    for (const std::shared_ptr<SceneObject>& object : scene_.GetObjects()) {
        if (!object || !object->HasMesh()) {
            continue;
        }

        /*
         * Transform принадлежит всему SceneObject.
         *
         * Для импортированной модели это означает,
         * что все её material parts используют
         * одну Position / Rotation / Scale.
         */
        const Matrix4 model = object->GetTransform().GetModelMatrix();

        shader_->SetMat4("uModel", model);

        const Matrix4 view_model = AffineTransformation::Multiply4(view, model);

        const Matrix4 mvp = AffineTransformation::Multiply4(projection, view_model);

        /*
         * Одна функция используется и для обычных
         * объектов с одним Mesh, и для импортированной
         * модели с несколькими RenderPart.
         */
        const auto draw_part = [this, &object, &mvp](const Mesh& mesh, const Material& material) {
            Vec3 render_color = material.GetColor();

            /*
             * Немного подсвечиваем выбранный SceneObject.
             *
             * Если модель состоит из нескольких частей,
             * подсветятся все её части.
             */
            if (object == selected_object_) {
                render_color.x = std::min(render_color.x + 0.15f, 1.0f);
                render_color.y = std::min(render_color.y + 0.10f, 1.0f);
            }

            shader_->SetVec4("uColor", Vec4{ render_color.x, render_color.y, render_color.z, 1.0f });

            shader_->SetFloat("uAmbientStrength", material.GetAmbientStrength());

            shader_->SetFloat("uDiffuseStrength", material.GetDiffuseStrength());

            shader_->SetFloat("uSpecularStrength", material.GetSpecularStrength());

            shader_->SetFloat("uShininess", material.GetShininess());

            if (material.HasDiffuseTexture()) {
                material.GetDiffuseTexture()->Bind(0);
                shader_->SetInt("uDiffuseTexture", 0);
                shader_->SetInt("uHasDiffuseTexture", 1);
            } else {
                shader_->SetInt("uHasDiffuseTexture", 0);
            }

            renderer_.Draw(mesh, *shader_, mvp);
        };

        /*
         * Импортированная многоматериальная модель.
         *
         * SceneObject один, но внутри находится
         * несколько Mesh + Material.
         */
        if (object->HasRenderParts()) {
            for (const SceneObject::SceneRenderPart& part : object->GetRenderParts()) {
                if (!part.mesh) {
                    continue;
                }

                draw_part(*part.mesh, part.material);
            }

            continue;
        }

        /*
         * Старый вариант SceneObject.
         *
         * Он остаётся нужен для Cube, Plane, Sphere
         * и других объектов с одним Mesh.
         */
        const std::shared_ptr<const Mesh> mesh = object->GetMesh();

        if (mesh) {
            draw_part(*mesh, object->GetMaterial());
        }
    }

    /*
     * Дальше освещение basic shader уже не нужно.
     */
    shader_->SetInt("uLightingEnabled", 0);
    shader_->SetInt("uHasDiffuseTexture", 0);

    /**
     * @brief Рисует editor-маркеры всех PointLight в Scene.
     *
     * Маркер является только визуальным представлением
     * источника света в Editor и не является Mesh
     * самого SceneObject.
     *
     * Даже выключенный PointLight остаётся видимым:
     * Enabled управляет освещением, а не существованием
     * объекта в Editor.
     */
    if (light_shader_ && light_mesh_) {
        light_shader_->Use();

        for (const std::shared_ptr<SceneObject>& object : scene_.GetObjects()) {
            if (!object || !object->HasPointLight()) {
                continue;
            }

            const PointLight* light = object->GetPointLight();

            if (!light) {
                continue;
            }

            const Vec3& light_position = object->GetTransform().position;

            const Matrix4 light_model =
                AffineTransformation::Translation4(
                    light_position.x,
                    light_position.y,
                    light_position.z
                );

            const Matrix4 light_view_model =
                AffineTransformation::Multiply4(
                    view,
                    light_model
                );

            const Matrix4 light_mvp =
                AffineTransformation::Multiply4(
                    projection,
                    light_view_model
                );

            light_shader_->SetVec3(
                "uLightColor",
                light->GetColor()
            );

            renderer_.Draw(
                *light_mesh_,
                *light_shader_,
                light_mvp
            );
        }
    }

    /*
     * После lamp shader возвращаем основной shader,
     * потому что Gizmo рисуется через него.
     */
    shader_->Use();
    shader_->SetInt("uLightingEnabled", 0);
    shader_->SetInt("uHasDiffuseTexture", 0);

    /*
     * Transform Gizmo.
     */
    if (gizmo_visible_ && selected_object_) {
        const Transform& selected_transform = selected_object_->GetTransform();
        const Vec3 gizmo_position = selected_transform.position;

        /*
         * Move и Rotate Gizmo пока отображаются в World Space.
         *
         * Scale Gizmo работает в Local Space:
         * если SceneObject уже повёрнут, оси масштабирования
         * должны повторять его Rotation, а не оставаться
         * направленными по глобальным X / Y / Z.
         *
         * Текущий Scale объекта на размер самого Gizmo
         * намеренно не влияет.
         */
        Matrix4 gizmo_model = AffineTransformation::Translation4(
            gizmo_position.x,
            gizmo_position.y,
            gizmo_position.z
        );

        if (gizmo_mode_ == GizmoMode::Scale) {
            Transform scale_gizmo_transform = selected_transform;
            scale_gizmo_transform.scale = Vec3{1.0f, 1.0f, 1.0f};
            gizmo_model = scale_gizmo_transform.GetModelMatrix();
        }

        const Matrix4 gizmo_view_model = AffineTransformation::Multiply4(view, gizmo_model);
        const Matrix4 gizmo_mvp = AffineTransformation::Multiply4(projection, gizmo_view_model);

        const auto draw_gizmo_part = [this, &gizmo_mvp](const Mesh& mesh, const Vec4& color, float width) {
            shader_->SetVec4("uColor", color);
            renderer_.DrawLines(mesh, *shader_, gizmo_mvp, width);
        };

        if (gizmo_mode_ == GizmoMode::Rotate) {
            if (rotate_gizmo_x_mesh_) {
                draw_gizmo_part(*rotate_gizmo_x_mesh_, Vec4{0.95f, 0.25f, 0.25f, 1.0f}, 4.0f);
            }

            if (rotate_gizmo_y_mesh_) {
                draw_gizmo_part(*rotate_gizmo_y_mesh_, Vec4{0.30f, 0.90f, 0.35f, 1.0f}, 4.0f);
            }

            if (rotate_gizmo_z_mesh_) {
                draw_gizmo_part(*rotate_gizmo_z_mesh_, Vec4{0.25f, 0.50f, 1.0f, 1.0f}, 4.0f);
            }
        } else {
            const float line_width = gizmo_mode_ == GizmoMode::Scale ? 6.0f : 4.0f;

            if (gizmo_x_mesh_) {
                draw_gizmo_part(*gizmo_x_mesh_, Vec4{0.95f, 0.25f, 0.25f, 1.0f}, line_width);
            }

            if (gizmo_y_mesh_) {
                draw_gizmo_part(*gizmo_y_mesh_, Vec4{0.30f, 0.90f, 0.35f, 1.0f}, line_width);
            }

            if (gizmo_z_mesh_) {
                draw_gizmo_part(*gizmo_z_mesh_, Vec4{0.25f, 0.50f, 1.0f, 1.0f}, line_width);
            }
        }
    }

}

void SceneViewport::SetDisplayedFile(const QString& file_path) {
    current_file_path_ = file_path;
    pending_model_path_ = file_path;

    /*
     * Если OpenGL ещё не инициализирован,
     * путь просто сохраняется.
     *
     * Реальный импорт произойдёт из initializeGL().
     */
    if (!gl_initialized_) {
        return;
    }

    /*
     * ModelImporter создаёт Mesh и Texture2D,
     * поэтому во время импорта OpenGL Context
     * обязательно должен быть активен.
     */
    makeCurrent();

    ImportPendingModel();

    doneCurrent();

    setFocus();
    UpdateProjectionTitle();
    UpdateCoordinatesLabel();
    update();
}

Scene& SceneViewport::GetScene() {
    return scene_;
}

const Scene& SceneViewport::GetScene() const {
    return scene_;
}

void SceneViewport::SetSelectedObject(std::shared_ptr<SceneObject> object) {
    /*
     * Этот метод может быть вызван
     * не только самим viewport,
     * но и, например, HierarchyPanel.
     *
     * Поэтому callback здесь намеренно
     * не вызывается — иначе легко получить
     * цикл:
     *
     * Hierarchy -> Viewport -> Hierarchy -> ...
     */
    selected_object_ =
        std::move(object);

    UpdateCoordinatesLabel();

    update();
}

std::shared_ptr<SceneObject> SceneViewport::GetSelectedObject() const {
    return selected_object_;
}

/**
 * @brief Удаляет выбранный SceneObject.
 *
 * PointLight не требует специальной обработки,
 * потому что его компонент принадлежит самому SceneObject.
 */
void SceneViewport::DeleteSelectedObject() {
    if (!selected_object_) {
        return;
    }

    const bool removed =
        scene_.RemoveObject(
            selected_object_
        );

    if (!removed) {
        return;
    }

    selected_object_.reset();

    UpdateProjectionTitle();
    UpdateCoordinatesLabel();
    NotifySelectionChanged();

    update();
}

void SceneViewport::SetSelectionChangedCallback(SelectionChangedCallback callback) {
    selection_changed_callback_ =
        std::move(callback);
}

void SceneViewport::NotifySelectionChanged() {
    if (!selection_changed_callback_) {
        return;
    }

    selection_changed_callback_(selected_object_);
}

void SceneViewport::SetProjectionMode(ProjectionMode mode) {
    projection_mode_ =
        mode;

    UpdateProjectionTitle();

    update();
}

SceneViewport::ProjectionMode SceneViewport::GetProjectionMode() const {
    return projection_mode_;
}

void SceneViewport::ImportPendingModel() {
    if (pending_model_path_.isEmpty()) {
        return;
    }

    const QString model_path = pending_model_path_;
    pending_model_path_.clear();

    std::vector<std::shared_ptr<SceneObject>> objects =
        ModelImporter::ImportObj(model_path.toStdString(), texture_manager_);

    if (objects.empty()) {
        const QFileInfo file_info(model_path);

        content_label_->show();

        content_label_->setText("Failed to load model\n\n" + file_info.fileName() + "\n\n" + model_path);

        return;
    }

    const Vec3 spawn_position = FindSpawnPosition();

    for (const std::shared_ptr<SceneObject>& object : objects) {
        if (!object) {
            continue;
        }

        object->SetType(SceneObject::Type::ImportedModel);
        object->SetSourcePath(model_path.toStdString());

        Transform& transform = object->GetTransform();

        transform.position = spawn_position;
        transform.rotation = Vec3{0.0f, 0.0f, 0.0f};
        transform.scale = Vec3{1.0f, 1.0f, 1.0f};

        scene_.AddObject(object);
    }

    content_label_->hide();

    selected_object_ = objects.front();

    UpdateProjectionTitle();
    UpdateCoordinatesLabel();
    NotifySelectionChanged();
    update();
}

Vec3 SceneViewport::FindSpawnPosition() const {
    constexpr float spacing = 0.9f;

    const std::size_t object_count =
        scene_.
            GetObjects().
            size();

    if (object_count == 0) {
        return Vec3{
            0.0f,
            0.0f,
            0.0f
        };
    }

    /*
     * Последовательность:
     *
     * 0
     * +0.9
     * -0.9
     * +1.8
     * -1.8
     * ...
     */
    const std::size_t pair_index =
        (object_count + 1) /
        2;

    const float direction =
        object_count % 2 == 1
            ? 1.0f
            : -1.0f;

    return Vec3{
        direction *
            static_cast<float>(pair_index) *
            spacing,

        0.0f,

        0.0f
    };
}

void SceneViewport::ArrangeSceneObjects() {
    const auto& objects =
        scene_.GetObjects();

    if (objects.empty()) {
        return;
    }

    constexpr float spacing = 0.9f;

    const float total_width =
        static_cast<float>(objects.size() - 1) *
        spacing;

    const float start_x =
        -total_width *
        0.5f;

    for (std::size_t index = 0; index < objects.size(); ++index) {
        if (!objects[index]) {
            continue;
        }

        Transform& transform =
            objects[index]->
                GetTransform();

        transform.position.x =
            start_x +
            static_cast<float>(index) *
            spacing;
    }
}

void SceneViewport::ApplyModelFit(const std::vector<std::shared_ptr<SceneObject>>& objects, const Vec3& spawn_position) {

    bool has_bounds = false;

    Vec3 minimum{};
    Vec3 maximum{};

    for (const std::shared_ptr<SceneObject>& object : objects) {
        if (!object) {
            continue;
        }

        const BoundingBox& box = object->GetBoundingBox();

        const Vec3 center = box.GetCenter();
        const Vec3 size = box.GetSize();

        const Vec3 box_minimum{
            center.x - size.x * 0.5f,
            center.y - size.y * 0.5f,
            center.z - size.z * 0.5f
        };

        const Vec3 box_maximum{
            center.x + size.x * 0.5f,
            center.y + size.y * 0.5f,
            center.z + size.z * 0.5f
        };

        if (!has_bounds) {
            minimum = box_minimum;
            maximum = box_maximum;
            has_bounds = true;
            continue;
        }

        minimum.x = std::min(minimum.x, box_minimum.x);
        minimum.y = std::min(minimum.y, box_minimum.y);
        minimum.z = std::min(minimum.z, box_minimum.z);

        maximum.x = std::max(maximum.x, box_maximum.x);
        maximum.y = std::max(maximum.y, box_maximum.y);
        maximum.z = std::max(maximum.z, box_maximum.z);
    }

    if (!has_bounds) {
        return;
    }

    const Vec3 center{
        (minimum.x + maximum.x) * 0.5f,
        (minimum.y + maximum.y) * 0.5f,
        (minimum.z + maximum.z) * 0.5f
    };

    const float size_x = maximum.x - minimum.x;
    const float size_y = maximum.y - minimum.y;
    const float size_z = maximum.z - minimum.z;

    const float maximum_size = std::max({ size_x, size_y, size_z });

    float fit_scale = 1.0f;

    if (maximum_size > kVectorEpsilon) {
        fit_scale = 0.6f / maximum_size;
    }

    const Vec3 model_position{
        spawn_position.x - center.x * fit_scale,
        spawn_position.y - center.y * fit_scale,
        spawn_position.z - center.z * fit_scale
    };

    const Vec3 model_scale{
        fit_scale,
        fit_scale,
        fit_scale
    };

    for (const std::shared_ptr<SceneObject>& object : objects) {
        if (!object) {
            continue;
        }

        Transform& transform = object->GetTransform();

        transform.position = model_position;
        transform.rotation = Vec3{0.0f, 0.0f, 0.0f};
        transform.scale = model_scale;
    }
}
/**
 * @brief Обрабатывает непрерывное движение Camera.
 *
 * Состояние WASD/QE задаётся в keyPressEvent/keyReleaseEvent,
 * а этот метод выполняет фактическое перемещение Camera.
 */
void SceneViewport::TickInput() {
    float delta_time = static_cast<float>(input_clock_.restart()) / 1000.0f;

    /*
     * Не позволяем большому delta time телепортировать Camera,
     * например после остановки приложения debugger'ом.
     */
    delta_time = std::min(delta_time, 0.05f);

    constexpr float move_speed = 2.0f;
    const float distance = move_speed * delta_time;

    if (move_forward_) {
        camera_.MoveForward(distance);
    }

    if (move_backward_) {
        camera_.MoveBackward(distance);
    }

    if (move_left_) {
        camera_.MoveLeft(distance);
    }

    if (move_right_) {
        camera_.MoveRight(distance);
    }

    if (move_up_) {
        camera_.MoveUp(distance);
    }

    if (move_down_) {
        camera_.MoveDown(distance);
    }
}

void SceneViewport::UpdateProjectionTitle() {
    if (title_label_ == nullptr) {
        return;
    }

    const QString projection_name =
        projection_mode_ ==
        ProjectionMode::Perspective
            ? "Perspective"
            : "Orthographic";

    title_label_->setText(QString("Scene • %1 • Objects: %2") .arg(projection_name) .arg(scene_.GetObjects().size()));
}

/**
 * @brief Сбрасывает состояние управления Camera
 * при потере фокуса viewport.
 *
 * Это предотвращает "залипание" клавиш,
 * если пользователь отпустил кнопку после перехода
 * в другой элемент Editor.
 */
void SceneViewport::focusOutEvent(QFocusEvent* event) {
    ResetInputState();
    input_clock_.restart();

    QOpenGLWidget::focusOutEvent(
        event
    );
}

void SceneViewport::UpdateCoordinatesLabel() {
    if (coordinates_label_ == nullptr) {
        return;
    }

    const Vec3& camera_position =
        camera_.GetPosition();

    QString text =
        QString("CAMERA\n" "X %1\n" "Y %2\n" "Z %3")
        .arg(camera_position.x, 0, 'f', 3)
        .arg(camera_position.y, 0, 'f', 3)
        .arg(camera_position.z, 0, 'f', 3);

    /*
     * Если объект выбран,
     * дополнительно показываем его Position.
     */
    if (selected_object_) {
        const Transform& transform =
            selected_object_->
                GetTransform();

        text +=
            QString("\n\nSELECTED\n" "%1\n" "X %2\n" "Y %3\n" "Z %4")
            .arg(QString::fromStdString(selected_object_->GetName()))
            .arg(transform.position.x, 0, 'f', 3)
            .arg(transform.position.y, 0, 'f', 3)
            .arg(transform.position.z, 0, 'f', 3);
    }

    coordinates_label_->setText(text);

    coordinates_label_->adjustSize();

    coordinates_label_->move(std::max(10, width() - coordinates_label_->width() - 14), 48);
}

Ray SceneViewport::CreateMouseRay(const QPointF& mouse_position) const {
    /*
     * Без реального размера viewport
     * построить корректный Ray невозможно.
     */
    if (width() <= 0 || height() <= 0) {
        return Ray{};
    }

    /*
     * Qt использует экранные координаты:
     *
     * (0,0) --------> X
     *   |
     *   |
     *   v
     *   Y
     *
     * Для NDC нужны координаты:
     *
     * X: [-1, +1]
     * Y: [-1, +1]
     *
     * причём +Y направлен вверх.
     */
    const float ndc_x =
        2.0f *
            static_cast<float>(mouse_position.x()) /
            static_cast<float>(width()) -
        1.0f;

    const float ndc_y =
        1.0f -
        2.0f *
            static_cast<float>(mouse_position.y()) /
            static_cast<float>(height());

    const float aspect =
        static_cast<float>(width()) /
        static_cast<float>(height());

    /*
     * Базис Camera в World Space.
     */
    const Vec3 forward =
        camera_.GetForward();

    const Vec3 right =
        camera_.GetRight();

    const Vec3 up =
        camera_.GetUp();

    /*
     * Perspective и Orthographic создают Ray
     * по-разному.
     */
    if (projection_mode_ == ProjectionMode::Perspective) {
        /*
         * У Perspective все лучи начинаются
         * в позиции Camera.
         *
         * Отличается только direction.
         */
        const float half_fov_radians =
            kPerspectiveFovDegrees *
            0.5f *
            kPi /
            180.0f;

        const float half_height =
            std::tan(half_fov_radians);

        const float half_width =
            half_height *
            aspect;

        Vec3 direction{
            forward.x +
                right.x *
                    ndc_x *
                    half_width +
                up.x *
                    ndc_y *
                    half_height,

            forward.y +
                right.y *
                    ndc_x *
                    half_width +
                up.y *
                    ndc_y *
                    half_height,

            forward.z +
                right.z *
                    ndc_x *
                    half_width +
                up.z *
                    ndc_y *
                    half_height
        };

        direction =
            Normalize(direction);

        return Ray{
            camera_.GetPosition(),
            direction
        };
    }

    /*
     * Orthographic:
     *
     * все лучи параллельны друг другу,
     * но имеют разное начало.
     */
    const float half_height =
        orthographic_half_height_;

    const float half_width =
        half_height *
        aspect;

    const Vec3 camera_position =
        camera_.GetPosition();

    const Vec3 origin{
        camera_position.x +
            right.x *
                ndc_x *
                half_width +
            up.x *
                ndc_y *
                half_height,

        camera_position.y +
            right.y *
                ndc_x *
                half_width +
            up.y *
                ndc_y *
                half_height,

        camera_position.z +
            right.z *
                ndc_x *
                half_width +
            up.z *
                ndc_y *
                half_height
    };

    return Ray{
        origin,
        Normalize(forward)
    };
}

void SceneViewport::SelectObjectAt(const QPointF& mouse_position) {
    /*
     * Создаём Ray в World Space.
     */
    const Ray world_ray =
        CreateMouseRay(mouse_position);

    /*
     * Пока подходящего объекта нет.
     */
    std::shared_ptr<SceneObject>
        nearest_object;

    /*
     * Храним расстояние до ближайшего
     * пересечения в World Space.
     */
    float nearest_world_distance =
        std::numeric_limits<float>::
            infinity();

    /*
     * Проверяем каждый SceneObject.
     */
    for (const std::shared_ptr<SceneObject>& object : scene_.GetObjects()) {
        if (!object) {
            continue;
        }

        /*
         * Point Light выбирается по editor-сфере.
         *
         * Это позволяет кликнуть непосредственно по источнику
         * во Viewport так же, как по обычной модели.
         */
        if (object->GetType() == SceneObject::Type::PointLight) {
            float light_distance = 0.0f;

            constexpr float light_selection_radius = 0.22f;

            if (IntersectRayWithSphere(
                    world_ray,
                    object->GetTransform().position,
                    light_selection_radius,
                    light_distance
                )) {
                if (light_distance < nearest_world_distance) {
                    nearest_world_distance = light_distance;
                    nearest_object = object;
                }
                }

            continue;
        }

        if (!object->HasMesh()) {
            continue;
        }

        const Transform& transform =
            object->GetTransform();

        /*
         * Если хотя бы один компонент Scale равен нулю,
         * Model Matrix необратима.
         *
         * Такой объект нельзя корректно перевести
         * из World Space обратно в Local Space.
         */
        if (std::abs(transform.scale.x) < kVectorEpsilon || std::abs(transform.scale.y) < kVectorEpsilon || std::abs(transform.scale.z) < kVectorEpsilon) {
            continue;
        }

        /*
         * Model Matrix:
         *
         * Local -> World
         */
        const Matrix4 model =
            transform.
                GetModelMatrix();

        /*
         * Inverse Model Matrix:
         *
         * World -> Local
         */
        const Matrix4 inverse_model =
            AffineTransformation::
                InverseAffine(model);

        /*
         * BoundingBox хранится
         * в Local Space объекта.
         *
         * Поэтому вместо преобразования Box
         * переводим сам Ray в Local Space.
         */
        Ray local_ray{};

        /*
         * origin — это точка,
         * поэтому translation применяется.
         */
        local_ray.origin =
            AffineTransformation::
                TransformPoint(inverse_model, world_ray.origin);

        /*
         * direction — это направление,
         * поэтому translation НЕ применяется.
         *
         * ВАЖНО:
         *
         * Здесь специально НЕ нормализуем
         * local_ray.direction.
         *
         * При наличии Scale длина направления
         * изменяется. Это нормально и сохраняет
         * параметризацию Ray после аффинного
         * преобразования.
         */
        local_ray.direction =
            AffineTransformation::
                TransformDirection(inverse_model, world_ray.direction);

        float local_distance =
            0.0f;

        /*
         * Broad Phase:
         *
         * проверяем пересечение не со всеми
         * треугольниками Mesh, а с его AABB.
         */
        const bool intersects =
            local_ray.Intersects(object->GetBoundingBox(), local_distance);

        if (!intersects) {
            continue;
        }

        /*
         * Получаем точку попадания
         * внутри Local Space.
         */
        const Vec3 local_hit_point =
            local_ray.GetPoint(local_distance);

        /*
         * Переводим точку попадания
         * обратно в World Space.
         */
        const Vec3 world_hit_point =
            AffineTransformation::
                TransformPoint(model, local_hit_point);

        /*
         * Теперь расстояние можно честно
         * сравнивать между разными объектами,
         * даже если у них различный Scale.
         */
        const float world_distance =
            Distance(world_ray.origin, world_hit_point);

        if (world_distance < nearest_world_distance) {
            nearest_world_distance =
                world_distance;

            nearest_object =
                object;
        }
    }

    /*
     * Если ни один BoundingBox не пересечён,
     * nearest_object останется nullptr,
     * то есть клик по пустому месту
     * снимает selection.
     */
    selected_object_ =
        std::move(nearest_object);

    UpdateCoordinatesLabel();

    NotifySelectionChanged();

    update();
}

void SceneViewport::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        return;
    }

    switch (event->key()) {
        case Qt::Key_W:
            move_forward_ = true;
            event->accept();
            return;

        case Qt::Key_S:
            move_backward_ = true;
            event->accept();
            return;

        case Qt::Key_A:
            move_left_ = true;
            event->accept();
            return;

        case Qt::Key_D:
            move_right_ = true;
            event->accept();
            return;

        case Qt::Key_E:
            move_up_ = true;
            event->accept();
            return;

        case Qt::Key_Q:
            move_down_ = true;
            event->accept();
            return;

        case Qt::Key_1:
        case Qt::Key_P:
            SetProjectionMode(ProjectionMode::Perspective);
            break;

        case Qt::Key_2:
        case Qt::Key_O:
            SetProjectionMode(ProjectionMode::Orthographic);
            break;

        case Qt::Key_3:
            gizmo_mode_ = GizmoMode::Move;
            gizmo_visible_ = true;
            update();
            break;

        case Qt::Key_4:
            gizmo_mode_ = GizmoMode::Rotate;
            gizmo_visible_ = true;
            update();
            break;

        case Qt::Key_5:
            gizmo_mode_ = GizmoMode::Scale;
            gizmo_visible_ = true;
            update();
            break;

        case Qt::Key_H:
            gizmo_visible_ = !gizmo_visible_;
            update();
            break;

        case Qt::Key_L:
            lighting_enabled_ = !lighting_enabled_;
            update();
            break;

        case Qt::Key_F:
            FrameSelectedObject();
            break;

        case Qt::Key_G:
            grid_visible_ = !grid_visible_;
            update();
            break;

        case Qt::Key_Escape:
            selected_object_.reset();

            UpdateCoordinatesLabel();
            NotifySelectionChanged();

            update();
            break;

        case Qt::Key_R:
            if (selected_object_) {
                Transform& transform = selected_object_->GetTransform();

                transform.position = Vec3{0.0f, 0.0f, 0.0f};
                transform.rotation = Vec3{0.0f, 0.0f, 0.0f};
                transform.scale = Vec3{1.0f, 1.0f, 1.0f};

                if (transform_changed_callback_) {
                    transform_changed_callback_();
                }

                UpdateCoordinatesLabel();
                update();
            }
            break;

        case Qt::Key_X:
            axes_visible_ = !axes_visible_;
            update();
            break;

        case Qt::Key_C:
            coordinates_visible_ = !coordinates_visible_;

            if (coordinates_label_) {
                coordinates_label_->setVisible(coordinates_visible_);
            }
            break;

        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            DeleteSelectedObject();
            break;

        default:
            QOpenGLWidget::keyPressEvent(event);
            break;
    }
}

void SceneViewport::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        return;
    }

    switch (event->key()) {
        case Qt::Key_W:
            move_forward_ =
                false;
            break;

        case Qt::Key_S:
            move_backward_ =
                false;
            break;

        case Qt::Key_A:
            move_left_ =
                false;
            break;

        case Qt::Key_D:
            move_right_ =
                false;
            break;

        case Qt::Key_E:
            move_up_ =
                false;
            break;

        case Qt::Key_Q:
            move_down_ =
                false;
            break;

        default:
            QOpenGLWidget::
                keyReleaseEvent(event);
            break;
    }
}

void SceneViewport::mousePressEvent(QMouseEvent* event) {
    setFocus();

    /*
     * Alt + ЛКМ:
     *
     * управление поворотом Camera.
     *
     * Camera имеет больший приоритет,
     * чем Move Gizmo.
     */
    if (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::AltModifier)) {
        pointer_look_active_ =
            true;

        last_pointer_position_ =
            event->position();

        event->accept();

        return;
    }

    /*
     * Обычный ЛКМ.
     */
    if (event->button() == Qt::LeftButton) {
        /*
         * Сначала проверяем Move Gizmo.
         *
         * Если пользователь нажал на одну
         * из его осей, SceneObject повторно
         * выбирать не нужно.
         */
        if (TryBeginGizmoDrag(event->position())) {
            event->accept();
            return;
        }

        /*
         * Если ни одна ось gizmo
         * не была нажата —
         * выполняем обычный Ray Picking.
         */
        SelectObjectAt(event->position());

        event->accept();

        return;
    }

    QOpenGLWidget::
        mousePressEvent(event);
}

void SceneViewport::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        /*
         * Завершаем перемещение объекта
         * через Move Gizmo.
         */
        if (gizmo_drag_active_) {
            gizmo_drag_active_ =
                false;

            active_gizmo_axis_ =
                GizmoAxis::None;

            update();

            event->accept();

            return;
        }

        /*
         * Завершаем вращение Camera.
         */
        if (pointer_look_active_) {
            pointer_look_active_ =
                false;

            event->accept();

            return;
        }
    }

    QOpenGLWidget::
        mouseReleaseEvent(event);
}

void SceneViewport::mouseMoveEvent(QMouseEvent* event) {
    const QPointF current_position =
        event->position();

    /*
     * Изменение Transform выбранного SceneObject через Gizmo.
     */
    if (gizmo_drag_active_ && selected_object_) {
        const QPointF delta = current_position - last_pointer_position_;
        Transform& transform = selected_object_->GetTransform();

        if (gizmo_mode_ == GizmoMode::Move) {
            constexpr float move_sensitivity = 0.01f;

            switch (active_gizmo_axis_) {
                case GizmoAxis::X:
                    transform.position.x += static_cast<float>(delta.x()) * move_sensitivity;
                    break;

                case GizmoAxis::Y:
                    transform.position.y -= static_cast<float>(delta.y()) * move_sensitivity;
                    break;

                case GizmoAxis::Z:
                    transform.position.z += static_cast<float>(delta.x()) * move_sensitivity;
                    break;

                case GizmoAxis::None:
                    break;
            }
        }

        if (gizmo_mode_ == GizmoMode::Rotate) {
            constexpr float rotate_sensitivity = 0.5f;
            const float angle_delta = static_cast<float>(delta.x() - delta.y()) * rotate_sensitivity;

            switch (active_gizmo_axis_) {
                case GizmoAxis::X:
                    transform.rotation.x += angle_delta;
                    break;

                case GizmoAxis::Y:
                    transform.rotation.y += angle_delta;
                    break;

                case GizmoAxis::Z:
                    transform.rotation.z += angle_delta;
                    break;

                case GizmoAxis::None:
                    break;
            }
        }

        if (gizmo_mode_ == GizmoMode::Scale) {
            constexpr float scale_sensitivity = 0.01f;
            const float scale_delta = static_cast<float>(delta.x() - delta.y()) * scale_sensitivity;

            switch (active_gizmo_axis_) {
                case GizmoAxis::X:
                    transform.scale.x = std::max(0.01f, transform.scale.x + scale_delta);
                    break;

                case GizmoAxis::Y:
                    transform.scale.y = std::max(0.01f, transform.scale.y + scale_delta);
                    break;

                case GizmoAxis::Z:
                    transform.scale.z = std::max(0.01f, transform.scale.z + scale_delta);
                    break;

                case GizmoAxis::None:
                    break;
            }
        }

        last_pointer_position_ = current_position;

        if (transform_changed_callback_) {
            transform_changed_callback_();
        }

        UpdateCoordinatesLabel();
        update();

        event->accept();
        return;
    }

    /*
     * Alt + ЛКМ:
     * вращение Camera.
     */
    if (pointer_look_active_) {
        const QPointF delta =
            current_position -
            last_pointer_position_;

        constexpr float mouse_sensitivity = 0.20f;

        camera_.Rotate(static_cast<float>(delta.x()) * mouse_sensitivity, static_cast<float>(-delta.y()) * mouse_sensitivity);

        last_pointer_position_ =
            current_position;

        UpdateCoordinatesLabel();

        update();

        event->accept();

        return;
    }

    QOpenGLWidget::
        mouseMoveEvent(event);
}

void SceneViewport::wheelEvent(QWheelEvent* event) {
    float scroll_y =
        0.0f;

    /*
     * Trackpad обычно сообщает pixelDelta.
     */
    if (!event->pixelDelta().isNull()) {
        scroll_y =
            static_cast<float>(event->pixelDelta().y());

        scroll_y *=
            0.006f;
    } else {
        /*
         * Обычное колесо мыши
         * обычно сообщает angleDelta.
         */
        scroll_y =
            static_cast<float>(event->angleDelta().y()) /
            120.0f;

        scroll_y *=
            0.25f;
    }

    if (projection_mode_ == ProjectionMode::Perspective) {
        /*
         * Perspective:
         *
         * колесо двигает Camera
         * вдоль Forward.
         */
        camera_.MoveForward(scroll_y);
    } else {
        /*
         * Orthographic:
         *
         * Camera не обязана физически
         * приближаться к сцене.
         *
         * Вместо этого меняется размер
         * ортографической области.
         */
        orthographic_half_height_ -=
            scroll_y *
            0.5f;

        orthographic_half_height_ =
            std::clamp(orthographic_half_height_, 0.1f, 50.0f);
    }

    UpdateCoordinatesLabel();

    update();

    event->accept();
}

void SceneViewport::FrameSelectedObject() {
    if (!selected_object_) {
        return;
    }

    const BoundingBox& bounding_box =
        selected_object_->GetBoundingBox();

    const Vec3 local_center =
        bounding_box.GetCenter();

    const Vec3 local_size =
        bounding_box.GetSize();

    const Matrix4 model =
        selected_object_->
            GetTransform().
            GetModelMatrix();

    /*
     * BoundingBox хранится в локальных координатах Mesh,
     * поэтому центр переводим в мировое пространство.
     */
    const Vec3 world_center =
        AffineTransformation::TransformPoint(model, local_center);

    /*
     * Для оценки размера объекта учитываем Scale.
     *
     * Берём максимальный размер по трём осям,
     * чтобы камера гарантированно отодвинулась
     * достаточно далеко даже для вытянутых моделей.
     */
    const Vec3 scale =
        selected_object_->
            GetTransform().
            scale;

    const float world_size_x =
        std::abs(local_size.x * scale.x);

    const float world_size_y =
        std::abs(local_size.y * scale.y);

    const float world_size_z =
        std::abs(local_size.z * scale.z);

    const float max_size =
        std::max({ world_size_x, world_size_y, world_size_z });

    /*
     * Используем половину максимального размера
     * как приближённый радиус объекта.
     */
    const float radius =
        std::max(max_size * 0.5f, 0.1f);

    float distance =
        radius * 2.5f;

    /*
     * Для Perspective учитываем угол обзора.
     */
    if (projection_mode_ == ProjectionMode::Perspective) {
        const float fov_radians =
            kPerspectiveFovDegrees *
            kPi /
            180.0f;

        distance =
            radius /
            std::tan(fov_radians * 0.5f);

        /*
         * Небольшой запас, чтобы объект
         * не упирался в края viewport.
         */
        distance *=
            1.35f;
    } else {
        /*
         * В Orthographic размер объекта зависит
         * не от расстояния до камеры,
         * а от orthographic_half_height_.
         */
        orthographic_half_height_ =
            std::max(radius * 1.5f, 0.5f);

        distance =
            radius * 2.5f;
    }

    const Vec3 forward =
        camera_.GetForward();

    const Vec3 new_camera_position{
        world_center.x -
            forward.x *
            distance,

        world_center.y -
            forward.y *
            distance,

        world_center.z -
            forward.z *
            distance
    };

    camera_.SetPosition(new_camera_position);

    UpdateCoordinatesLabel();

    update();
}

/**
 * @brief Определяет выбранную ось Move или Scale Gizmo.
 *
 * Move Gizmo использует мировые оси X / Y / Z.
 * Scale Gizmo использует локальные оси SceneObject:
 * после Rotation визуальные оси и picking поворачиваются
 * вместе с выбранным объектом.
 *
 * @param mouse_position Положение курсора в Scene View.
 * @return Выбранная ось Gizmo или None.
 */
SceneViewport::GizmoAxis SceneViewport::PickMoveScaleGizmoAxis(const QPointF& mouse_position) const {
    if (!selected_object_) {
        return GizmoAxis::None;
    }

    const Ray ray = CreateMouseRay(mouse_position);
    const Transform& transform = selected_object_->GetTransform();
    const Vec3 origin = transform.position;

    constexpr float gizmo_length = 1.0f;
    constexpr float selection_radius = 0.12f;

    Vec3 x_direction{1.0f, 0.0f, 0.0f};
    Vec3 y_direction{0.0f, 1.0f, 0.0f};
    Vec3 z_direction{0.0f, 0.0f, 1.0f};

    /*
     * Для Scale поворачиваем базовые локальные оси
     * матрицей Transform выбранного SceneObject.
     *
     * Scale принудительно ставим в 1, чтобы длина
     * editor-осей не зависела от масштаба объекта.
     */
    if (gizmo_mode_ == GizmoMode::Scale) {
        Transform scale_gizmo_transform = transform;
        scale_gizmo_transform.scale = Vec3{1.0f, 1.0f, 1.0f};

        const Matrix4 local_axis_matrix = scale_gizmo_transform.GetModelMatrix();

        x_direction = Normalize(AffineTransformation::TransformDirection(local_axis_matrix, x_direction));
        y_direction = Normalize(AffineTransformation::TransformDirection(local_axis_matrix, y_direction));
        z_direction = Normalize(AffineTransformation::TransformDirection(local_axis_matrix, z_direction));
    }

    const Vec3 x_end{
        origin.x + x_direction.x * gizmo_length,
        origin.y + x_direction.y * gizmo_length,
        origin.z + x_direction.z * gizmo_length
    };

    const Vec3 y_end{
        origin.x + y_direction.x * gizmo_length,
        origin.y + y_direction.y * gizmo_length,
        origin.z + y_direction.z * gizmo_length
    };

    const Vec3 z_end{
        origin.x + z_direction.x * gizmo_length,
        origin.y + z_direction.y * gizmo_length,
        origin.z + z_direction.z * gizmo_length
    };

    const float x_distance = DistanceRayToSegment(ray, origin, x_end);
    const float y_distance = DistanceRayToSegment(ray, origin, y_end);
    const float z_distance = DistanceRayToSegment(ray, origin, z_end);

    float best_distance = selection_radius;
    GizmoAxis result = GizmoAxis::None;

    if (x_distance < best_distance) {
        best_distance = x_distance;
        result = GizmoAxis::X;
    }

    if (y_distance < best_distance) {
        best_distance = y_distance;
        result = GizmoAxis::Y;
    }

    if (z_distance < best_distance) {
        result = GizmoAxis::Z;
    }

    return result;
}

SceneViewport::GizmoAxis SceneViewport::PickRotateGizmoAxis(const QPointF& mouse_position) const {
    if (!selected_object_) {
        return GizmoAxis::None;
    }

    const Ray ray = CreateMouseRay(mouse_position);
    const Vec3 origin = selected_object_->GetTransform().position;

    constexpr int segment_count = 64;
    constexpr float radius = 1.0f;
    constexpr float selection_radius = 0.12f;

    const auto distance_to_ring = [&ray, &origin](GizmoAxis axis) {
        float best_distance = std::numeric_limits<float>::infinity();

        for (int index = 0; index < segment_count; ++index) {
            const float first_angle = 2.0f * kPi * static_cast<float>(index) / static_cast<float>(segment_count);
            const float second_angle = 2.0f * kPi * static_cast<float>(index + 1) / static_cast<float>(segment_count);

            Vec3 first{};
            Vec3 second{};

            if (axis == GizmoAxis::X) {
                first = Vec3{origin.x, origin.y + std::cos(first_angle) * radius, origin.z + std::sin(first_angle) * radius};
                second = Vec3{origin.x, origin.y + std::cos(second_angle) * radius, origin.z + std::sin(second_angle) * radius};
            }

            if (axis == GizmoAxis::Y) {
                first = Vec3{origin.x + std::cos(first_angle) * radius, origin.y, origin.z + std::sin(first_angle) * radius};
                second = Vec3{origin.x + std::cos(second_angle) * radius, origin.y, origin.z + std::sin(second_angle) * radius};
            }

            if (axis == GizmoAxis::Z) {
                first = Vec3{origin.x + std::cos(first_angle) * radius, origin.y + std::sin(first_angle) * radius, origin.z};
                second = Vec3{origin.x + std::cos(second_angle) * radius, origin.y + std::sin(second_angle) * radius, origin.z};
            }

            best_distance = std::min(best_distance, DistanceRayToSegment(ray, first, second));
        }

        return best_distance;
    };

    const float x_distance = distance_to_ring(GizmoAxis::X);
    const float y_distance = distance_to_ring(GizmoAxis::Y);
    const float z_distance = distance_to_ring(GizmoAxis::Z);

    float best_distance = selection_radius;
    GizmoAxis result = GizmoAxis::None;

    if (x_distance < best_distance) {
        best_distance = x_distance;
        result = GizmoAxis::X;
    }

    if (y_distance < best_distance) {
        best_distance = y_distance;
        result = GizmoAxis::Y;
    }

    if (z_distance < best_distance) {
        result = GizmoAxis::Z;
    }

    return result;
}

bool SceneViewport::TryBeginGizmoDrag(const QPointF& mouse_position) {
    if (!gizmo_visible_ || !selected_object_) {
        return false;
    }

    if (gizmo_mode_ == GizmoMode::Rotate) {
        active_gizmo_axis_ = PickRotateGizmoAxis(mouse_position);
    } else {
        active_gizmo_axis_ = PickMoveScaleGizmoAxis(mouse_position);
    }

    if (active_gizmo_axis_ == GizmoAxis::None) {
        return false;
    }

    gizmo_drag_active_ = true;
    last_pointer_position_ = mouse_position;

    update();
    return true;
}

void SceneViewport::SetTransformChangedCallback(TransformChangedCallback callback) {
    transform_changed_callback_ =
        std::move(callback);
}

void SceneViewport::CreateCube() {
    CreatePrimitive("Cube", PrimitiveGenerator::CreateCube(), SceneObject::Type::Cube);
}

void SceneViewport::CreatePlane() {
    CreatePrimitive("Plane", PrimitiveGenerator::CreatePlane(), SceneObject::Type::Plane);
}

void SceneViewport::CreateSphere() {
    CreatePrimitive("Sphere", PrimitiveGenerator::CreateSphere(), SceneObject::Type::Sphere);
}

void SceneViewport::CreatePrimitive(const QString& name, ImportedMeshData mesh_data, SceneObject::Type type) {
    if (!gl_initialized_) {
        return;
    }

    makeCurrent();

    const BoundingBox bounding_box = BoundingBox::FromPoints(mesh_data.positions);
    auto mesh = std::make_shared<Mesh>(mesh_data);
    auto object = std::make_shared<SceneObject>(name.toStdString(), std::move(mesh));

    object->SetType(type);
    object->SetBoundingBox(bounding_box);
    object->GetTransform().position = FindSpawnPosition();

    scene_.AddObject(object);

    content_label_->hide();

    doneCurrent();

    selected_object_ = object;

    UpdateProjectionTitle();
    UpdateCoordinatesLabel();
    NotifySelectionChanged();

    update();
}

/**
 * @brief Полностью очищает Scene.
 *
 * PointLight является обычным SceneObject с компонентом
 * PointLight, поэтому scene_.Clear() удаляет источники
 * света вместе с остальными объектами.
 */
void SceneViewport::ClearScene() {
    ResetInputState();
    scene_.Clear();

    selected_object_.reset();
    pending_model_path_.clear();
    current_file_path_.clear();

    UpdateProjectionTitle();
    UpdateCoordinatesLabel();
    NotifySelectionChanged();

    if (content_label_) {
        content_label_->show();
        content_label_->setText("Scene is empty");
    }

    update();
}

void SceneViewport::ResetInputState() {
    move_forward_ = false;
    move_backward_ = false;
    move_left_ = false;
    move_right_ = false;
    move_up_ = false;
    move_down_ = false;

    pointer_look_active_ = false;
    gizmo_drag_active_ = false;
    active_gizmo_axis_ = GizmoAxis::None;
}

void SceneViewport::SetMaterialTexture(Material& material, const QString& path) {
    if (path.isEmpty()) {
        return;
    }

    makeCurrent();

    std::shared_ptr<Texture2D> texture = texture_manager_.Load(path.toStdString());

    if (texture) {
        material.SetDiffuseTexture(std::move(texture));
        material.SetDiffuseTexturePath(path.toStdString());
    }

    doneCurrent();

    update();
}

bool SceneViewport::LoadScene(const QString& file_path) {
    if (file_path.isEmpty() || !gl_initialized_) {
        return false;
    }

    makeCurrent();

    const bool loaded = SceneSerializer::Load(scene_, file_path.toStdString(), texture_manager_);

    doneCurrent();

    if (!loaded) {
        return false;
    }

    selected_object_.reset();

    if (scene_.GetObjects().empty()) {
        content_label_->show();
        content_label_->setText("Scene is empty");
    } else {
        content_label_->hide();
    }

    UpdateProjectionTitle();
    UpdateCoordinatesLabel();

    setFocus();
    update();

    return true;
}

/**
 * @brief Создаёт Point Light и сразу выбирает его.
 */
void SceneViewport::CreatePointLight() {
    CreatePointLightObject(true);
}

/**
 * @brief Создаёт полноценный SceneObject источника света.
 *
 * Каждый вызов создаёт независимый PointLight.
 *
 * @param select_object Нужно ли сразу выбрать новый объект.
 */
void SceneViewport::CreatePointLightObject(bool select_object) {
    std::size_t light_count = 0;

    for (const std::shared_ptr<SceneObject>& object : scene_.GetObjects()) {
        if (object && object->HasPointLight()) {
            ++light_count;
        }
    }

    std::string name = "Point Light";

    if (light_count > 0) {
        name += " (" + std::to_string(light_count) + ")";
    }

    auto object = std::make_shared<SceneObject>(name);

    object->SetType(
        SceneObject::Type::PointLight
    );

    /*
     * PointLight хранит только параметры освещения.
     * Положение источника берётся из Transform SceneObject.
     */
    PointLight light;

    object->SetPointLight(
        light
    );

    /*
     * Первый источник появляется в привычном месте.
     *
     * Последующие немного смещаются,
     * чтобы не находиться строго друг внутри друга.
     */
    object->GetTransform().position =
        Vec3{
            2.0f + static_cast<float>(light_count) * 1.5f,
            3.0f,
            2.0f
        };

    scene_.AddObject(
        object
    );

    content_label_->hide();

    if (select_object) {
        selected_object_ = object;

        NotifySelectionChanged();
        UpdateCoordinatesLabel();
    }

    UpdateProjectionTitle();

    update();
}

bool SceneViewport::StartTest(TestScene test_scene, ExecutionMode execution_mode) {
    if (!gl_initialized_) {
        return false;
    }

    if (test_running_) {
        return false;
    }

    /*
     * Реальный MultiThreaded путь мы ещё не написали.
     *
     * Не притворяемся, что он уже существует.
     */
    if (execution_mode == ExecutionMode::MultiThreaded) {
        return false;
    }

    makeCurrent();

    /*
     * Сохраняем обычную пользовательскую Scene.
     *
     * После Stop Test она будет восстановлена.
     */
    scene_before_test_.emplace(std::move(scene_));

    scene_ = Scene{};

    selected_object_.reset();

    switch (test_scene) {
        case TestScene::Cube1k: {
            const ImportedMeshData cube_data = PrimitiveGenerator::CreateCube();

            std::shared_ptr<Mesh> cube_mesh = std::make_shared<Mesh>(cube_data);

            Cube1kTest::Create(scene_, cube_mesh);

            break;
        }

        default: {
            scene_ = std::move(scene_before_test_.value());
            scene_before_test_.reset();

            doneCurrent();

            return false;
        }
    }

    doneCurrent();

    execution_mode_ = execution_mode;

    test_running_ = true;

    /*
     * Следующий paintGL будет первым кадром benchmark.
     */
    first_frame_ = true;

    if (content_label_) {
        content_label_->hide();
    }

    UpdateProjectionTitle();
    UpdateCoordinatesLabel();
    NotifySelectionChanged();

    update();

    return true;
}

void SceneViewport::StopTest() {
    if (!test_running_) {
        return;
    }

    /*
     * GPU Mesh тестовой сцены необходимо удалить,
     * пока OpenGL context активен.
     */
    makeCurrent();

    scene_.Clear();

    doneCurrent();

    /*
     * Возвращаем пользовательскую Scene.
     */
    if (scene_before_test_.has_value()) {
        scene_ = std::move(scene_before_test_.value());
        scene_before_test_.reset();
    }

    selected_object_.reset();

    execution_mode_ = ExecutionMode::SingleThreaded;

    test_running_ = false;

    first_frame_ = true;

    if (content_label_) {
        if (scene_.GetObjects().empty()) {
            content_label_->show();
            content_label_->setText("Scene is empty");
        } else {
            content_label_->hide();
        }
    }

    UpdateProjectionTitle();
    UpdateCoordinatesLabel();
    NotifySelectionChanged();

    update();
}

bool SceneViewport::IsTestRunning() const {
    return test_running_;
}