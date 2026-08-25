#include "Editor/Viewport/SceneViewport.h"

#include "Engine/Assets/ObjParser.h"
#include "Engine/Math/affine_transformation.h"
#include "Engine/Math/projection.h"
#include "Engine/Platform/OpenGL/OpenGLLoader.h"

#include <QByteArray>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QWheelEvent>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <utility>


namespace {

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

} // namespace


SceneViewport::SceneViewport(
    QWidget* parent
)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(
        Qt::StrongFocus
    );

    setMouseTracking(true);

    CreateLayout();

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
            update();
        }
    );

    input_timer_.start(16);
}


SceneViewport::~SceneViewport()
{
    if (context() != nullptr) {
        makeCurrent();

        mesh_.reset();
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

    title_label_ =
        new QLabel(
            "Scene",
            this
        );

    title_label_->setFixedHeight(30);

    title_label_->setAlignment(
        Qt::AlignVCenter |
        Qt::AlignLeft
    );

    title_label_->setContentsMargins(
        10,
        0,
        0,
        0
    );

    title_label_->setStyleSheet(
        R"(
            background-color: #292929;
            color: #dddddd;
            border-bottom: 1px solid #3a3a3a;
            font-weight: 600;
        )"
    );

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
            background-color: #1e1e1e;
            color: #909090;
            font-size: 17px;
        )"
    );

    UpdateProjectionTitle();
}


void SceneViewport::initializeGL()
{
    if (!InitializeOpenGLLoader(
            &GetQtOpenGLProcAddress
        )) {
        throw std::runtime_error(
            "Failed to initialize GLAD for Qt OpenGL context"
        );
    }

    renderer_.Initialize();

    const std::filesystem::path
        shader_directory =
            MINI_ENGINE_SHADER_DIR;

    shader_ =
        std::make_unique<Shader>(
            shader_directory /
                "basic.vert",

            shader_directory /
                "basic.frag"
        );

    gl_initialized_ = true;

    UploadPendingMesh();
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
            30
        );
    }

    if (content_label_ != nullptr) {
        content_label_->setGeometry(
            0,
            30,
            width,
            std::max(
                0,
                height - 30
            )
        );
    }
}


void SceneViewport::paintGL()
{
    renderer_.BeginFrame(
        background_color_
    );

    if (!mesh_ || !shader_) {
        return;
    }

    const Matrix4 model =
        AffineTransformation::
            ComposeModelMatrix(
                model_position_,
                model_rotation_,
                model_scale_
            );

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

    if (
        projection_mode_ ==
        ProjectionMode::Perspective
    ) {
        projection =
            Projection::Perspective(
                45.0f,
                aspect,
                0.1f,
                100.0f
            );
    } else {
        const float half_height =
            orthographic_half_height_;

        const float half_width =
            half_height * aspect;

        projection =
            Projection::Ortho(
                -half_width,
                half_width,
                -half_height,
                half_height,
                0.1f,
                100.0f
            );
    }

    const Matrix4 view_model =
        AffineTransformation::
            Multiply4(
                view,
                model
            );

    const Matrix4 mvp =
        AffineTransformation::
            Multiply4(
                projection,
                view_model
            );

    shader_->Use();

    shader_->SetVec4(
        "uColor",
        Vec4{
            0.72f,
            0.78f,
            0.90f,
            1.0f
        }
    );

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

    renderer_.Draw(
        *mesh_,
        *shader_,
        mvp
    );
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

    CalculateModelFit(
        mesh_data
    );

    pending_mesh_data_ =
        std::move(mesh_data);

    if (gl_initialized_) {
        makeCurrent();

        UploadPendingMesh();

        doneCurrent();
    }

    content_label_->hide();

    // После открытия модели WASD сразу работает
    // без дополнительного клика по viewport.
    setFocus();

    update();
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

    mesh_ =
        std::make_unique<Mesh>(
            *pending_mesh_data_
        );

    pending_mesh_data_.reset();
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

    const Vec3 center{
        (minimum.x + maximum.x) *
            0.5f,

        (minimum.y + maximum.y) *
            0.5f,

        (minimum.z + maximum.z) *
            0.5f
    };

    const float size_x =
        maximum.x - minimum.x;

    const float size_y =
        maximum.y - minimum.y;

    const float size_z =
        maximum.z - minimum.z;

    const float maximum_size =
        std::max({
            size_x,
            size_y,
            size_z
        });

    float fit_scale =
        1.0f;

    if (maximum_size > 0.000001f) {
        fit_scale =
            1.5f /
            maximum_size;
    }

    model_position_ =
        Vec3{
            -center.x * fit_scale,
            -center.y * fit_scale,
            -center.z * fit_scale
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
     * Если приложение долго было остановлено debugger'ом
     * или свернуто, не позволяем следующему кадру
     * телепортировать камеру.
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

    if (
        projection_mode_ ==
        ProjectionMode::Perspective
    ) {
        title_label_->setText(
            "Scene  •  Perspective"
        );
    } else {
        title_label_->setText(
            "Scene  •  Orthographic"
        );
    }
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
            move_forward_ = true;
            break;

        case Qt::Key_S:
            move_backward_ = true;
            break;

        case Qt::Key_A:
            move_left_ = true;
            break;

        case Qt::Key_D:
            move_right_ = true;
            break;

        case Qt::Key_E:
            move_up_ = true;
            break;

        case Qt::Key_Q:
            move_down_ = true;
            break;

        case Qt::Key_1:
        case Qt::Key_P:
            SetProjectionMode(
                ProjectionMode::Perspective
            );
            break;

        case Qt::Key_2:
        case Qt::Key_O:
            SetProjectionMode(
                ProjectionMode::Orthographic
            );
            break;

        default:
            QOpenGLWidget::
                keyPressEvent(event);
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
            move_forward_ = false;
            break;

        case Qt::Key_S:
            move_backward_ = false;
            break;

        case Qt::Key_A:
            move_left_ = false;
            break;

        case Qt::Key_D:
            move_right_ = false;
            break;

        case Qt::Key_E:
            move_up_ = false;
            break;

        case Qt::Key_Q:
            move_down_ = false;
            break;

        default:
            QOpenGLWidget::
                keyReleaseEvent(event);
            break;
    }
}


void SceneViewport::mousePressEvent(
    QMouseEvent* event
)
{
    setFocus();

    if (
        event->button() ==
        Qt::LeftButton
    ) {
        pointer_look_active_ =
            true;

        last_pointer_position_ =
            event->position();

        event->accept();
        return;
    }

    QOpenGLWidget::
        mousePressEvent(event);
}


void SceneViewport::mouseReleaseEvent(
    QMouseEvent* event
)
{
    if (
        event->button() ==
        Qt::LeftButton
    ) {
        pointer_look_active_ =
            false;

        event->accept();
        return;
    }

    QOpenGLWidget::
        mouseReleaseEvent(event);
}


void SceneViewport::mouseMoveEvent(
    QMouseEvent* event
)
{
    if (!pointer_look_active_) {
        QOpenGLWidget::
            mouseMoveEvent(event);

        return;
    }

    const QPointF current_position =
        event->position();

    const QPointF delta =
        current_position -
        last_pointer_position_;

    last_pointer_position_ =
        current_position;

    /*
     * Для MacBook trackpad физическое движение пальца
     * относительно небольшое, поэтому чувствительность
     * делаем умеренной.
     */
    constexpr float look_sensitivity =
        0.18f;

    camera_.Rotate(
        static_cast<float>(
            delta.x()
        ) * look_sensitivity,

        static_cast<float>(
            -delta.y()
        ) * look_sensitivity
    );

    update();

    event->accept();
}


void SceneViewport::wheelEvent(
    QWheelEvent* event
)
{
    /*
     * На Mac trackpad pixelDelta() обычно содержит
     * высокоточное двухпальцевое движение.
     *
     * У обычной мыши чаще доступен angleDelta().
     */
    float scroll_y = 0.0f;

    if (!event->pixelDelta().isNull()) {
        scroll_y =
            static_cast<float>(
                event->pixelDelta().y()
            );

        scroll_y *= 0.006f;
    } else {
        scroll_y =
            static_cast<float>(
                event->angleDelta().y()
            ) /
            120.0f;

        scroll_y *= 0.25f;
    }

    if (
        projection_mode_ ==
        ProjectionMode::Perspective
    ) {
        camera_.MoveForward(
            scroll_y
        );
    } else {
        /*
         * В orthographic projection движение камеры
         * вперёд не изменяет видимый размер объекта.
         *
         * Поэтому zoom меняет размер самого
         * orthographic volume.
         */
        orthographic_half_height_ -=
            scroll_y * 0.5f;

        orthographic_half_height_ =
            std::clamp(
                orthographic_half_height_,
                0.1f,
                50.0f
            );
    }

    update();

    event->accept();
}