#include "Editor/Viewport/SceneViewport.h"

#include "Engine/Assets/ObjParser.h"
#include "Engine/Math/affine_transformation.h"
#include "Engine/Math/projection.h"
#include "Engine/Platform/OpenGL/OpenGLLoader.h"
#include "Engine/Scene/BoundingBox.h"

#include <QByteArray>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>


namespace {

/**
 * @brief Вертикальный угол обзора Perspective-камеры.
 *
 * Значение должно совпадать с FOV,
 * используемым при построении Projection Matrix.
 */
constexpr float kPerspectiveFovDegrees =
    45.0f;


/**
 * @brief Ближняя плоскость отсечения.
 */
constexpr float kNearPlane =
    0.1f;


/**
 * @brief Дальняя плоскость отсечения.
 */
constexpr float kFarPlane =
    100.0f;


/**
 * @brief Число PI для перевода градусов в радианы.
 */
constexpr float kPi =
    3.14159265358979323846f;


/**
 * @brief Минимальное значение,
 * используемое при проверках float.
 */
constexpr float kVectorEpsilon =
    0.000001f;


/**
 * @brief Возвращает OpenGL-функцию
 * из текущего Qt OpenGL Context.
 *
 * Используется для инициализации GLAD,
 * не подключая GLAD непосредственно
 * внутрь SceneViewport.
 */
void* GetQtOpenGLProcAddress(
    const char* name
)
{
    QOpenGLContext* current_context =
        QOpenGLContext::currentContext();

    if (current_context == nullptr) {
        return nullptr;
    }

    const QFunctionPointer function =
        current_context->getProcAddress(
            QByteArray(name)
        );

    return reinterpret_cast<void*>(
        function
    );
}


/**
 * @brief Возвращает длину Vec3.
 */
float Length(
    const Vec3& vector
)
{
    return std::sqrt(
        vector.x * vector.x +
        vector.y * vector.y +
        vector.z * vector.z
    );
}


/**
 * @brief Нормализует Vec3.
 *
 * Если длина практически равна нулю,
 * возвращается нулевой вектор.
 */
Vec3 Normalize(
    const Vec3& vector
)
{
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
 * @brief Возвращает расстояние между двумя точками.
 */
float Distance(
    const Vec3& first,
    const Vec3& second
)
{
    const Vec3 difference{
        first.x - second.x,
        first.y - second.y,
        first.z - second.z
    };

    return Length(
        difference
    );
}

} // namespace


SceneViewport::SceneViewport(
    QWidget* parent
)
    : QOpenGLWidget(parent)
{
    /*
     * Viewport должен получать клавиатурный focus,
     * чтобы WASD / QE работали после клика по Scene.
     */
    setFocusPolicy(
        Qt::StrongFocus
    );

    /*
     * Позволяет получать mouseMoveEvent
     * даже без зажатой кнопки мыши.
     */
    setMouseTracking(true);

    CreateLayout();

    /*
     * Таймер используется для вычисления
     * delta time движения Camera.
     */
    input_clock_.start();

    input_timer_.setTimerType(
        Qt::PreciseTimer
    );

    connect(
        &input_timer_,
        &QTimer::timeout,
        this,
        [this]()
        {
            TickInput();

            UpdateCoordinatesLabel();

            update();
        }
    );

    /*
     * Примерно 60 обновлений в секунду.
     */
    input_timer_.start(16);
}


SceneViewport::~SceneViewport()
{
    /*
     * OpenGL-ресурсы должны уничтожаться,
     * пока соответствующий Context активен.
     */
    if (context() != nullptr) {
        makeCurrent();

        scene_.Clear();


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


        shader_.reset();

        doneCurrent();
    }
}


void SceneViewport::CreateLayout()
{
    setMinimumSize(
        500,
        400
    );


    /*
     * Верхняя строка Scene View.
     */
    title_label_ =
        new QLabel(
            "Scene",
            this
        );

    title_label_->setFixedHeight(
        34
    );

    title_label_->setAlignment(
        Qt::AlignVCenter |
        Qt::AlignLeft
    );

    title_label_->setContentsMargins(
        12,
        0,
        0,
        0
    );

    title_label_->setStyleSheet(
        R"(
            background-color: #3c3f41;
            color: #d7dae0;
            border-bottom: 1px solid #51555a;
            font-size: 13px;
            font-weight: 600;
        )"
    );


    /*
     * Сообщение, показываемое,
     * пока ни одной модели нет.
     */
    content_label_ =
        new QLabel(this);

    content_label_->setAlignment(
        Qt::AlignCenter
    );

    content_label_->setText(
        "Scene View\n\n"
        "Open a model using\n"
        "File → Open Model..."
    );

    content_label_->setStyleSheet(
        R"(
            background-color: #25282b;
            color: #a9adb3;
            font-size: 16px;
        )"
    );


    /*
     * Панель информации о Camera
     * и выбранном SceneObject.
     */
    coordinates_label_ =
        new QLabel(this);

    coordinates_label_->setAlignment(
        Qt::AlignRight |
        Qt::AlignTop
    );

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

    coordinates_label_->setText(
        "Camera\n"
        "X  0.000\n"
        "Y  0.000\n"
        "Z  3.000"
    );

    coordinates_label_->adjustSize();

    UpdateProjectionTitle();
}


void SceneViewport::initializeGL()
{
    /*
     * Загружаем OpenGL-функции
     * через текущий Qt OpenGL Context.
     */
    if (!InitializeOpenGLLoader(
            &GetQtOpenGLProcAddress
        )) {
        throw std::runtime_error(
            "Failed to initialize GLAD for Qt OpenGL context"
        );
    }


    renderer_.Initialize();


    /*
     * Загружаем основной Shader.
     */
    const std::filesystem::path shader_directory =
        MINI_ENGINE_SHADER_DIR;

    shader_ =
        std::make_unique<Shader>(
            shader_directory /
                "basic.vert",

            shader_directory /
                "basic.frag"
        );


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


    gl_initialized_ =
        true;


    /*
     * Если OBJ был открыт ещё до создания
     * OpenGL Context, загружаем его сейчас.
     */
    UploadPendingMesh();
}


void SceneViewport::CreateEditorGrid()
{
    grid_mesh_ =
        std::make_unique<Mesh>(
            CreateGridMeshData()
        );


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


ImportedMeshData
SceneViewport::CreateGridMeshData() const
{
    ImportedMeshData data;

    constexpr int half_grid =
        10;

    constexpr float step =
        1.0f;


    /*
     * Строим Grid на плоскости XZ.
     *
     * Центральные линии не рисуем,
     * потому что вместо них отдельно
     * будут нарисованы оси X и Z.
     */
    for (
        int index = -half_grid;
        index <= half_grid;
        ++index
    ) {
        if (index == 0) {
            continue;
        }


        const float coordinate =
            static_cast<float>(
                index
            ) *
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
            static_cast<std::uint32_t>(
                data.render_vertices.size()
            );


        data.render_vertices.push_back(
            first_z
        );

        data.render_vertices.push_back(
            second_z
        );


        data.render_indices.push_back(
            first_index
        );

        data.render_indices.push_back(
            first_index + 1
        );


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
            static_cast<std::uint32_t>(
                data.render_vertices.size()
            );


        data.render_vertices.push_back(
            first_x
        );

        data.render_vertices.push_back(
            second_x
        );


        data.render_indices.push_back(
            second_index
        );

        data.render_indices.push_back(
            second_index + 1
        );
    }


    return data;
}

void SceneViewport::CreateMoveGizmo()
{
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

ImportedMeshData
SceneViewport::CreateAxisMeshData(
    const Vec3& start,
    const Vec3& end
) const
{
    ImportedMeshData data;


    Vertex first{};

    first.position =
        start;


    Vertex second{};

    second.position =
        end;


    data.render_vertices.push_back(
        first
    );

    data.render_vertices.push_back(
        second
    );


    data.render_indices.push_back(
        0
    );

    data.render_indices.push_back(
        1
    );


    return data;
}


void SceneViewport::resizeGL(
    int width,
    int height
)
{
    renderer_.SetViewport(
        width,
        height
    );


    if (title_label_ != nullptr) {
        title_label_->setGeometry(
            0,
            0,
            width,
            34
        );
    }


    if (content_label_ != nullptr) {
        content_label_->setGeometry(
            0,
            34,
            width,
            std::max(
                0,
                height - 34
            )
        );
    }


    if (coordinates_label_ != nullptr) {
        coordinates_label_->adjustSize();

        coordinates_label_->move(
            std::max(
                10,
                width -
                    coordinates_label_->
                        width() -
                    14
            ),
            48
        );
    }
}


void SceneViewport::paintGL()
{
    renderer_.BeginFrame(
        background_color_
    );


    if (!shader_) {
        return;
    }


    const Matrix4 view =
        camera_.GetViewMatrix();


    const float aspect =
        height() > 0
            ? static_cast<float>(
                  width()
              ) /
              static_cast<float>(
                  height()
              )
            : 1.0f;


    Matrix4 projection{};


    /*
     * Projection Matrix должна использовать
     * те же параметры, что и CreateMouseRay().
     *
     * Иначе визуально объект будет находиться
     * в одном месте, а Ray Picking —
     * рассчитываться по другой геометрии камеры.
     */
    if (
        projection_mode_ ==
        ProjectionMode::Perspective
    ) {
        projection =
            Projection::Perspective(
                kPerspectiveFovDegrees,
                aspect,
                kNearPlane,
                kFarPlane
            );
    } else {
        const float half_height =
            orthographic_half_height_;

        const float half_width =
            half_height *
            aspect;


        projection =
            Projection::Ortho(
                -half_width,
                half_width,
                -half_height,
                half_height,
                kNearPlane,
                kFarPlane
            );
    }


    const Matrix4 view_projection =
        AffineTransformation::Multiply4(
            projection,
            view
        );


    shader_->Use();


    shader_->SetInt(
        "uPointMode",
        0
    );

    shader_->SetFloat(
        "uPointSize",
        6.0f
    );

    shader_->SetFloat(
        "uPointSoft",
        0.05f
    );

    shader_->SetFloat(
        "uDashFill",
        1.0f
    );


    /*
     * Grid.
     */
    if (grid_mesh_) {
        shader_->SetVec4(
            "uColor",
            Vec4{
                0.30f,
                0.32f,
                0.35f,
                1.0f
            }
        );

        renderer_.DrawLines(
            *grid_mesh_,
            *shader_,
            view_projection,
            1.0f
        );
    }


    /*
     * Ось X.
     */
    if (axis_x_mesh_) {
        shader_->SetVec4(
            "uColor",
            Vec4{
                0.82f,
                0.30f,
                0.30f,
                1.0f
            }
        );

        renderer_.DrawLines(
            *axis_x_mesh_,
            *shader_,
            view_projection,
            2.0f
        );
    }


    /*
     * Ось Z.
     */
    if (axis_z_mesh_) {
        shader_->SetVec4(
            "uColor",
            Vec4{
                0.30f,
                0.52f,
                0.90f,
                1.0f
            }
        );

        renderer_.DrawLines(
            *axis_z_mesh_,
            *shader_,
            view_projection,
            2.0f
        );
    }


    /*
     * Ось Y.
     */
    if (axis_y_mesh_) {
        shader_->SetVec4(
            "uColor",
            Vec4{
                0.40f,
                0.75f,
                0.42f,
                1.0f
            }
        );

        renderer_.DrawLines(
            *axis_y_mesh_,
            *shader_,
            view_projection,
            2.0f
        );
    }


    /*
     * Каждый SceneObject имеет собственный Transform,
     * поэтому для него строится отдельная MVP Matrix:
     *
     * MVP = Projection * View * Model
     */
    for (
        const std::shared_ptr<SceneObject>& object :
        scene_.GetObjects()
    ) {
        if (!object) {
            continue;
        }


        const std::shared_ptr<const Mesh> mesh =
            object->GetMesh();


        if (!mesh) {
            continue;
        }


        /*
         * Выбранный объект отображается
         * отдельным цветом.
         *
         * Пока это простой вариант визуального
         * выделения без дополнительного
         * outline-pass и stencil buffer.
         */
        if (
            object ==
            selected_object_
        ) {
            shader_->SetVec4(
                "uColor",
                Vec4{
                    1.0f,
                    0.68f,
                    0.20f,
                    1.0f
                }
            );
        } else {
            shader_->SetVec4(
                "uColor",
                Vec4{
                    0.67f,
                    0.76f,
                    0.91f,
                    1.0f
                }
            );
        }


        const Matrix4 model =
            object->
                GetTransform().
                GetModelMatrix();


        const Matrix4 view_model =
            AffineTransformation::Multiply4(
                view,
                model
            );


        const Matrix4 mvp =
            AffineTransformation::Multiply4(
                projection,
                view_model
            );


        renderer_.Draw(
            *mesh,
            *shader_,
            mvp
        );
    }

    /*
 * Move Gizmo отображается только тогда,
 * когда в сцене выбран объект.
 */
    if (
        selected_object_ &&
        gizmo_x_mesh_ &&
        gizmo_y_mesh_ &&
        gizmo_z_mesh_
    ) {
        /*
         * Gizmo должен находиться в origin
         * выбранного SceneObject.
         *
         * Для этого используем только Position объекта.
         * Rotation и Scale самого объекта на gizmo
         * пока не влияют: оси gizmo ориентированы
         * относительно мировых X, Y и Z.
         */
        const Vec3 gizmo_position =
            selected_object_->
                GetTransform().
                position;


        const Matrix4 gizmo_model =
            AffineTransformation::Translation4(
                gizmo_position.x,
                gizmo_position.y,
                gizmo_position.z
            );


        const Matrix4 gizmo_view_model =
            AffineTransformation::Multiply4(
                view,
                gizmo_model
            );


        const Matrix4 gizmo_mvp =
            AffineTransformation::Multiply4(
                projection,
                gizmo_view_model
            );


        /*
         * X — красная ось.
         */
        shader_->SetVec4(
            "uColor",
            Vec4{
                0.95f,
                0.25f,
                0.25f,
                1.0f
            }
        );

        renderer_.DrawLines(
            *gizmo_x_mesh_,
            *shader_,
            gizmo_mvp,
            4.0f
        );


        /*
         * Y — зелёная ось.
         */
        shader_->SetVec4(
            "uColor",
            Vec4{
                0.30f,
                0.90f,
                0.35f,
                1.0f
            }
        );

        renderer_.DrawLines(
            *gizmo_y_mesh_,
            *shader_,
            gizmo_mvp,
            4.0f
        );


        /*
         * Z — синяя ось.
         */
        shader_->SetVec4(
            "uColor",
            Vec4{
                0.25f,
                0.50f,
                1.0f,
                1.0f
            }
        );

        renderer_.DrawLines(
            *gizmo_z_mesh_,
            *shader_,
            gizmo_mvp,
            4.0f
        );
    }
}


void SceneViewport::SetDisplayedFile(
    const QString& file_path
)
{
    const QFileInfo file_info(
        file_path
    );


    current_file_path_ =
        file_path;


    ImportedMeshData mesh_data;


    const bool parsed =
        ObjParser::Parse(
            file_path.toStdString(),
            mesh_data
        );


    if (!parsed) {
        content_label_->show();

        content_label_->setText(
            "Failed to load model\n\n" +
            file_info.fileName() +
            "\n\n" +
            file_path
        );

        return;
    }


    /*
     * Рассчитываем стартовый Transform
     * импортированной модели.
     */
    CalculateModelFit(
        mesh_data
    );


    /*
     * Данные временно остаются на CPU,
     * пока не появится активный OpenGL Context.
     */
    pending_mesh_data_ =
        std::move(
            mesh_data
        );


    if (gl_initialized_) {
        makeCurrent();

        UploadPendingMesh();

        doneCurrent();
    }


    content_label_->hide();

    setFocus();

    UpdateProjectionTitle();

    UpdateCoordinatesLabel();

    update();
}


Scene& SceneViewport::GetScene()
{
    return scene_;
}


const Scene& SceneViewport::GetScene() const
{
    return scene_;
}


void SceneViewport::SetSelectedObject(
    std::shared_ptr<SceneObject> object
)
{
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
        std::move(
            object
        );


    UpdateCoordinatesLabel();

    update();
}


std::shared_ptr<SceneObject>
SceneViewport::GetSelectedObject() const
{
    return selected_object_;
}


void SceneViewport::SetSelectionChangedCallback(
    SelectionChangedCallback callback
)
{
    selection_changed_callback_ =
        std::move(
            callback
        );
}


void SceneViewport::NotifySelectionChanged()
{
    if (!selection_changed_callback_) {
        return;
    }


    selection_changed_callback_(
        selected_object_
    );
}


void SceneViewport::SetProjectionMode(
    ProjectionMode mode
)
{
    projection_mode_ =
        mode;


    UpdateProjectionTitle();

    update();
}


SceneViewport::ProjectionMode
SceneViewport::GetProjectionMode() const
{
    return projection_mode_;
}


void SceneViewport::UploadPendingMesh()
{
    if (!pending_mesh_data_.has_value()) {
        return;
    }


    /*
     * BoundingBox строится на CPU
     * из исходных координат Mesh.
     *
     * Важно:
     *
     * BoundingBox остаётся в Local Space.
     *
     * Transform SceneObject сюда
     * специально не применяется.
     */
    const BoundingBox bounding_box =
        BoundingBox::FromPoints(
            pending_mesh_data_->
                positions
        );


    /*
     * После этого геометрия передаётся на GPU.
     */
    auto mesh =
        std::make_shared<Mesh>(
            *pending_mesh_data_
        );


    const QFileInfo file_info(
        current_file_path_
    );


    auto object =
        std::make_shared<SceneObject>(
            file_info.
                completeBaseName().
                toStdString(),

            std::move(mesh)
        );


    /*
     * BoundingBox относится именно
     * к этому SceneObject.
     */
    object->SetBoundingBox(
        bounding_box
    );


    Transform& transform =
        object->GetTransform();


    transform.position =
        model_position_;

    transform.rotation =
        model_rotation_;

    transform.scale =
        model_scale_;


    /*
     * Находим стартовую позицию
     * только для нового SceneObject.
     *
     * Уже существующие объекты
     * при импорте не передвигаются.
     */
    const Vec3 spawn_position =
        FindSpawnPosition();


    transform.position.x +=
        spawn_position.x;

    transform.position.y +=
        spawn_position.y;

    transform.position.z +=
        spawn_position.z;


    scene_.AddObject(
        std::move(
            object
        )
    );


    UpdateProjectionTitle();

    pending_mesh_data_.reset();
}


Vec3 SceneViewport::FindSpawnPosition() const
{
    constexpr float spacing =
        0.9f;


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
            static_cast<float>(
                pair_index
            ) *
            spacing,

        0.0f,

        0.0f
    };
}


void SceneViewport::ArrangeSceneObjects()
{
    const auto& objects =
        scene_.GetObjects();


    if (objects.empty()) {
        return;
    }


    constexpr float spacing =
        0.9f;


    const float total_width =
        static_cast<float>(
            objects.size() - 1
        ) *
        spacing;


    const float start_x =
        -total_width *
        0.5f;


    for (
        std::size_t index = 0;
        index < objects.size();
        ++index
    ) {
        if (!objects[index]) {
            continue;
        }


        Transform& transform =
            objects[index]->
                GetTransform();


        transform.position.x =
            start_x +
            static_cast<float>(
                index
            ) *
            spacing;
    }
}


void SceneViewport::CalculateModelFit(
    const ImportedMeshData& mesh_data
)
{
    if (mesh_data.positions.empty()) {
        model_position_ =
            Vec3{};


        model_scale_ =
            Vec3{
                1.0f,
                1.0f,
                1.0f
            };


        return;
    }


    Vec3 minimum =
        mesh_data.positions.front();


    Vec3 maximum =
        mesh_data.positions.front();


    /*
     * Находим AABB исходной модели.
     */
    for (
        const Vec3& position :
        mesh_data.positions
    ) {
        minimum.x =
            std::min(
                minimum.x,
                position.x
            );

        minimum.y =
            std::min(
                minimum.y,
                position.y
            );

        minimum.z =
            std::min(
                minimum.z,
                position.z
            );


        maximum.x =
            std::max(
                maximum.x,
                position.x
            );

        maximum.y =
            std::max(
                maximum.y,
                position.y
            );

        maximum.z =
            std::max(
                maximum.z,
                position.z
            );
    }


    /*
     * Центр модели.
     */
    const Vec3 center{
        (minimum.x + maximum.x) *
            0.5f,

        (minimum.y + maximum.y) *
            0.5f,

        (minimum.z + maximum.z) *
            0.5f
    };


    const float size_x =
        maximum.x -
        minimum.x;


    const float size_y =
        maximum.y -
        minimum.y;


    const float size_z =
        maximum.z -
        minimum.z;


    const float maximum_size =
        std::max({
            size_x,
            size_y,
            size_z
        });


    float fit_scale =
        1.0f;


    /*
     * Масштабируем модель приблизительно
     * до размера 0.6 мировых единиц.
     */
    if (
        maximum_size >
        kVectorEpsilon
    ) {
        fit_scale =
            0.6f /
            maximum_size;
    }


    /*
     * После масштабирования центр модели
     * перемещается в начало координат.
     */
    model_position_ =
        Vec3{
            -center.x *
                fit_scale,

            -center.y *
                fit_scale,

            -center.z *
                fit_scale
        };


    model_rotation_ =
        Vec3{
            0.0f,
            0.0f,
            0.0f
        };


    model_scale_ =
        Vec3{
            fit_scale,
            fit_scale,
            fit_scale
        };
}


void SceneViewport::TickInput()
{
    float delta_time =
        static_cast<float>(
            input_clock_.restart()
        ) /
        1000.0f;


    /*
     * Ограничиваем слишком большой delta time,
     * например после остановки приложения
     * в debugger.
     */
    delta_time =
        std::min(
            delta_time,
            0.05f
        );


    constexpr float move_speed =
        2.0f;


    const float distance =
        move_speed *
        delta_time;


    if (move_forward_) {
        camera_.MoveForward(
            distance
        );
    }


    if (move_backward_) {
        camera_.MoveBackward(
            distance
        );
    }


    if (move_left_) {
        camera_.MoveLeft(
            distance
        );
    }


    if (move_right_) {
        camera_.MoveRight(
            distance
        );
    }


    if (move_up_) {
        camera_.MoveUp(
            distance
        );
    }


    if (move_down_) {
        camera_.MoveDown(
            distance
        );
    }
}


void SceneViewport::UpdateProjectionTitle()
{
    if (title_label_ == nullptr) {
        return;
    }


    const QString projection_name =
        projection_mode_ ==
        ProjectionMode::Perspective
            ? "Perspective"
            : "Orthographic";


    title_label_->setText(
        QString(
            "Scene  •  %1  •  Objects: %2"
        )
        .arg(
            projection_name
        )
        .arg(
            scene_.
                GetObjects().
                size()
        )
    );
}


void SceneViewport::UpdateCoordinatesLabel()
{
    if (coordinates_label_ == nullptr) {
        return;
    }


    const Vec3& camera_position =
        camera_.GetPosition();


    QString text =
        QString(
            "CAMERA\n"
            "X  %1\n"
            "Y  %2\n"
            "Z  %3"
        )
        .arg(
            camera_position.x,
            0,
            'f',
            3
        )
        .arg(
            camera_position.y,
            0,
            'f',
            3
        )
        .arg(
            camera_position.z,
            0,
            'f',
            3
        );


    /*
     * Если объект выбран,
     * дополнительно показываем его Position.
     */
    if (selected_object_) {
        const Transform& transform =
            selected_object_->
                GetTransform();


        text +=
            QString(
                "\n\nSELECTED\n"
                "%1\n"
                "X  %2\n"
                "Y  %3\n"
                "Z  %4"
            )
            .arg(
                QString::fromStdString(
                    selected_object_->
                        GetName()
                )
            )
            .arg(
                transform.position.x,
                0,
                'f',
                3
            )
            .arg(
                transform.position.y,
                0,
                'f',
                3
            )
            .arg(
                transform.position.z,
                0,
                'f',
                3
            );
    }


    coordinates_label_->setText(
        text
    );


    coordinates_label_->adjustSize();


    coordinates_label_->move(
        std::max(
            10,
            width() -
                coordinates_label_->
                    width() -
                14
        ),
        48
    );
}


Ray SceneViewport::CreateMouseRay(
    const QPointF& mouse_position
) const
{
    /*
     * Без реального размера viewport
     * построить корректный Ray невозможно.
     */
    if (
        width() <= 0 ||
        height() <= 0
    ) {
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
            static_cast<float>(
                mouse_position.x()
            ) /
            static_cast<float>(
                width()
            ) -
        1.0f;


    const float ndc_y =
        1.0f -
        2.0f *
            static_cast<float>(
                mouse_position.y()
            ) /
            static_cast<float>(
                height()
            );


    const float aspect =
        static_cast<float>(
            width()
        ) /
        static_cast<float>(
            height()
        );


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
    if (
        projection_mode_ ==
        ProjectionMode::Perspective
    ) {
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
            std::tan(
                half_fov_radians
            );


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
            Normalize(
                direction
            );


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
        Normalize(
            forward
        )
    };
}


void SceneViewport::SelectObjectAt(
    const QPointF& mouse_position
)
{
    /*
     * Создаём Ray в World Space.
     */
    const Ray world_ray =
        CreateMouseRay(
            mouse_position
        );


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
    for (
        const std::shared_ptr<SceneObject>& object :
        scene_.GetObjects()
    ) {
        if (!object) {
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
        if (
            std::abs(
                transform.scale.x
            ) <
                kVectorEpsilon ||

            std::abs(
                transform.scale.y
            ) <
                kVectorEpsilon ||

            std::abs(
                transform.scale.z
            ) <
                kVectorEpsilon
        ) {
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
                InverseAffine(
                    model
                );


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
                TransformPoint(
                    inverse_model,
                    world_ray.origin
                );


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
                TransformDirection(
                    inverse_model,
                    world_ray.direction
                );


        float local_distance =
            0.0f;


        /*
         * Broad Phase:
         *
         * проверяем пересечение не со всеми
         * треугольниками Mesh, а с его AABB.
         */
        const bool intersects =
            local_ray.Intersects(
                object->
                    GetBoundingBox(),

                local_distance
            );


        if (!intersects) {
            continue;
        }


        /*
         * Получаем точку попадания
         * внутри Local Space.
         */
        const Vec3 local_hit_point =
            local_ray.GetPoint(
                local_distance
            );


        /*
         * Переводим точку попадания
         * обратно в World Space.
         */
        const Vec3 world_hit_point =
            AffineTransformation::
                TransformPoint(
                    model,
                    local_hit_point
                );


        /*
         * Теперь расстояние можно честно
         * сравнивать между разными объектами,
         * даже если у них различный Scale.
         */
        const float world_distance =
            Distance(
                world_ray.origin,
                world_hit_point
            );


        if (
            world_distance <
            nearest_world_distance
        ) {
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
        std::move(
            nearest_object
        );


    UpdateCoordinatesLabel();

    NotifySelectionChanged();

    update();
}


void SceneViewport::keyPressEvent(
    QKeyEvent* event
)
{
    if (event->isAutoRepeat()) {
        return;
    }


    switch (event->key()) {
        case Qt::Key_W:
            move_forward_ =
                true;
            break;


        case Qt::Key_S:
            move_backward_ =
                true;
            break;


        case Qt::Key_A:
            move_left_ =
                true;
            break;


        case Qt::Key_D:
            move_right_ =
                true;
            break;


        case Qt::Key_E:
            move_up_ =
                true;
            break;


        case Qt::Key_Q:
            move_down_ =
                true;
            break;


        case Qt::Key_1:
        case Qt::Key_P:
            SetProjectionMode(
                ProjectionMode::
                    Perspective
            );
            break;


        case Qt::Key_2:
        case Qt::Key_O:
            SetProjectionMode(
                ProjectionMode::
                    Orthographic
            );
            break;

        case Qt::Key_F:
            FrameSelectedObject();
            break;


        default:
            QOpenGLWidget::
                keyPressEvent(
                    event
                );
            break;
    }
}


void SceneViewport::keyReleaseEvent(
    QKeyEvent* event
)
{
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
                keyReleaseEvent(
                    event
                );
            break;
    }
}

void SceneViewport::mousePressEvent(
    QMouseEvent* event
)
{
    setFocus();


    /*
     * Alt + ЛКМ:
     *
     * управление поворотом Camera.
     *
     * Camera имеет больший приоритет,
     * чем Move Gizmo.
     */
    if (
        event->button() ==
            Qt::LeftButton &&

        event->
            modifiers().
            testFlag(
                Qt::AltModifier
            )
    ) {
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
    if (
        event->button() ==
        Qt::LeftButton
    ) {
        /*
         * Сначала проверяем Move Gizmo.
         *
         * Если пользователь нажал на одну
         * из его осей, SceneObject повторно
         * выбирать не нужно.
         */
        if (
            TryBeginMoveGizmoDrag(
                event->position()
            )
        ) {
            event->accept();

            return;
        }


        /*
         * Если ни одна ось gizmo
         * не была нажата —
         * выполняем обычный Ray Picking.
         */
        SelectObjectAt(
            event->position()
        );


        event->accept();

        return;
    }


    QOpenGLWidget::
        mousePressEvent(
            event
        );
}

void SceneViewport::mouseReleaseEvent(
    QMouseEvent* event
)
{
    if (
        event->button() ==
        Qt::LeftButton
    ) {
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
        mouseReleaseEvent(
            event
        );
}

void SceneViewport::mouseMoveEvent(
    QMouseEvent* event
)
{
    const QPointF current_position =
        event->position();


    /*
     * Перемещение выбранного SceneObject
     * через Move Gizmo.
     */
    if (
        gizmo_drag_active_ &&
        selected_object_
    ) {
        const QPointF delta =
            current_position -
            last_pointer_position_;


        /*
         * Пока используем экранное смещение мыши
         * как величину перемещения.
         *
         * На следующем уровне развития gizmo
         * это можно заменить на точное
         * Ray/Axis преобразование.
         */
        constexpr float move_sensitivity =
            0.01f;


        Transform& transform =
            selected_object_->
                GetTransform();


        switch (
            active_gizmo_axis_
        ) {
            case GizmoAxis::X:
                transform.position.x +=
                    static_cast<float>(
                        delta.x()
                    ) *
                    move_sensitivity;
                break;


            case GizmoAxis::Y:
                /*
                 * Экранная координата Y в Qt
                 * увеличивается вниз,
                 * поэтому знак инвертируется.
                 */
                transform.position.y -=
                    static_cast<float>(
                        delta.y()
                    ) *
                    move_sensitivity;
                break;


            case GizmoAxis::Z:
                /*
                 * Для первого варианта Z
                 * управляется горизонтальным
                 * движением мыши.
                 */
                transform.position.z +=
                    static_cast<float>(
                        delta.x()
                    ) *
                    move_sensitivity;
                break;


            case GizmoAxis::None:
                break;
        }


        last_pointer_position_ =
            current_position;

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


        constexpr float mouse_sensitivity =
            0.20f;


        camera_.Rotate(
            static_cast<float>(
                delta.x()
            ) *
            mouse_sensitivity,

            static_cast<float>(
                -delta.y()
            ) *
            mouse_sensitivity
        );


        last_pointer_position_ =
            current_position;


        UpdateCoordinatesLabel();

        update();


        event->accept();

        return;
    }


    QOpenGLWidget::
        mouseMoveEvent(
            event
        );
}

void SceneViewport::wheelEvent(
    QWheelEvent* event
)
{
    float scroll_y =
        0.0f;


    /*
     * Trackpad обычно сообщает pixelDelta.
     */
    if (
        !event->
            pixelDelta().
            isNull()
    ) {
        scroll_y =
            static_cast<float>(
                event->
                    pixelDelta().
                    y()
            );


        scroll_y *=
            0.006f;
    } else {
        /*
         * Обычное колесо мыши
         * обычно сообщает angleDelta.
         */
        scroll_y =
            static_cast<float>(
                event->
                    angleDelta().
                    y()
            ) /
            120.0f;


        scroll_y *=
            0.25f;
    }


    if (
        projection_mode_ ==
        ProjectionMode::Perspective
    ) {
        /*
         * Perspective:
         *
         * колесо двигает Camera
         * вдоль Forward.
         */
        camera_.MoveForward(
            scroll_y
        );
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
            std::clamp(
                orthographic_half_height_,
                0.1f,
                50.0f
            );
    }


    UpdateCoordinatesLabel();

    update();

    event->accept();
}

void SceneViewport::FrameSelectedObject()
{
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
        AffineTransformation::TransformPoint(
            model,
            local_center
        );

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
        std::abs(
            local_size.x *
            scale.x
        );

    const float world_size_y =
        std::abs(
            local_size.y *
            scale.y
        );

    const float world_size_z =
        std::abs(
            local_size.z *
            scale.z
        );

    const float max_size =
        std::max(
            {
                world_size_x,
                world_size_y,
                world_size_z
            }
        );

    /*
     * Используем половину максимального размера
     * как приближённый радиус объекта.
     */
    const float radius =
        std::max(
            max_size * 0.5f,
            0.1f
        );

    float distance =
        radius * 2.5f;

    /*
     * Для Perspective учитываем угол обзора.
     */
    if (
        projection_mode_ ==
        ProjectionMode::Perspective
    ) {
        const float fov_radians =
            kPerspectiveFovDegrees *
            kPi /
            180.0f;

        distance =
            radius /
            std::tan(
                fov_radians *
                0.5f
            );

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
            std::max(
                radius * 1.5f,
                0.5f
            );

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

    camera_.SetPosition(
        new_camera_position
    );

    UpdateCoordinatesLabel();

    update();
}

SceneViewport::GizmoAxis
SceneViewport::PickMoveGizmoAxis(
    const QPointF& mouse_position
) const
{
    if (!selected_object_) {
        return GizmoAxis::None;
    }


    const Ray ray =
        CreateMouseRay(
            mouse_position
        );


    const Vec3 origin =
        selected_object_->
            GetTransform().
            position;


    /*
     * Gizmo пока имеет длину 1 world unit.
     */
    constexpr float gizmo_length =
        1.0f;


    /*
     * Допустимое расстояние от луча мыши
     * до оси gizmo.
     *
     * Позже сделаем размер gizmo независимым
     * от расстояния до Camera.
     */
    constexpr float selection_radius =
        0.12f;


    /*
     * Возвращает минимальное расстояние
     * между Ray и отрезком gizmo.
     */
    const auto distance_to_axis =
        [&ray](
            const Vec3& start,
            const Vec3& end
        )
        {
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


            const float a =
                ray.direction.x * ray.direction.x +
                ray.direction.y * ray.direction.y +
                ray.direction.z * ray.direction.z;


            const float b =
                ray.direction.x * segment.x +
                ray.direction.y * segment.y +
                ray.direction.z * segment.z;


            const float c =
                segment.x * segment.x +
                segment.y * segment.y +
                segment.z * segment.z;


            const float d =
                ray.direction.x * from_ray_to_segment.x +
                ray.direction.y * from_ray_to_segment.y +
                ray.direction.z * from_ray_to_segment.z;


            const float e =
                segment.x * from_ray_to_segment.x +
                segment.y * from_ray_to_segment.y +
                segment.z * from_ray_to_segment.z;


            const float denominator =
                a * c -
                b * b;


            float ray_parameter =
                0.0f;

            float segment_parameter =
                0.0f;


            if (
                std::abs(denominator) >
                1e-6f
            ) {
                ray_parameter =
                    (
                        b * e -
                        c * d
                    ) /
                    denominator;


                segment_parameter =
                    (
                        a * e -
                        b * d
                    ) /
                    denominator;
            }


            /*
             * Ray существует только вперёд
             * от позиции Camera.
             */
            ray_parameter =
                std::max(
                    ray_parameter,
                    0.0f
                );


            /*
             * Gizmo — конечный отрезок.
             */
            segment_parameter =
                std::clamp(
                    segment_parameter,
                    0.0f,
                    1.0f
                );


            const Vec3 point_on_ray{
                ray.origin.x +
                    ray.direction.x *
                    ray_parameter,

                ray.origin.y +
                    ray.direction.y *
                    ray_parameter,

                ray.origin.z +
                    ray.direction.z *
                    ray_parameter
            };


            const Vec3 point_on_segment{
                start.x +
                    segment.x *
                    segment_parameter,

                start.y +
                    segment.y *
                    segment_parameter,

                start.z +
                    segment.z *
                    segment_parameter
            };


            const float dx =
                point_on_ray.x -
                point_on_segment.x;

            const float dy =
                point_on_ray.y -
                point_on_segment.y;

            const float dz =
                point_on_ray.z -
                point_on_segment.z;


            return std::sqrt(
                dx * dx +
                dy * dy +
                dz * dz
            );
        };


    const Vec3 x_end{
        origin.x + gizmo_length,
        origin.y,
        origin.z
    };


    const Vec3 y_end{
        origin.x,
        origin.y + gizmo_length,
        origin.z
    };


    const Vec3 z_end{
        origin.x,
        origin.y,
        origin.z + gizmo_length
    };


    const float x_distance =
        distance_to_axis(
            origin,
            x_end
        );


    const float y_distance =
        distance_to_axis(
            origin,
            y_end
        );


    const float z_distance =
        distance_to_axis(
            origin,
            z_end
        );


    float best_distance =
        selection_radius;


    GizmoAxis result =
        GizmoAxis::None;


    if (
        x_distance <
        best_distance
    ) {
        best_distance =
            x_distance;

        result =
            GizmoAxis::X;
    }


    if (
        y_distance <
        best_distance
    ) {
        best_distance =
            y_distance;

        result =
            GizmoAxis::Y;
    }


    if (
        z_distance <
        best_distance
    ) {
        result =
            GizmoAxis::Z;
    }


    return result;
}


bool SceneViewport::TryBeginMoveGizmoDrag(
    const QPointF& mouse_position
)
{
    active_gizmo_axis_ =
        PickMoveGizmoAxis(
            mouse_position
        );


    if (
        active_gizmo_axis_ ==
        GizmoAxis::None
    ) {
        return false;
    }


    gizmo_drag_active_ =
        true;


    last_pointer_position_ =
        mouse_position;


    update();


    return true;
}


void SceneViewport::SetTransformChangedCallback(
    TransformChangedCallback callback
)
{
    transform_changed_callback_ =
        std::move(callback);
}
