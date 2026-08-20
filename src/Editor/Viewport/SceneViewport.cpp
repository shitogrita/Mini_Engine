#include "Editor/Viewport/SceneViewport.h"

#include "Engine/Assets/ObjParser.h"
#include "Engine/Math/affine_transformation.h"
#include "Engine/Math/projection.h"
#include "Engine/Platform/OpenGL/OpenGLLoader.h"

#include <QByteArray>
#include <QFileInfo>
#include <QLabel>
#include <QOpenGLContext>
#include <QKeyEvent>
#include <QTimer>

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

SceneViewport::SceneViewport(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    CreateLayout();

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
    /*
     * Mesh и Shader освобождают OpenGL-ресурсы
     * в своих деструкторах.
     *
     * Поэтому context этого QOpenGLWidget должен
     * быть current в момент их уничтожения.
     */
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
}


void SceneViewport::initializeGL()
{
    /*
     * QOpenGLWidget уже создал OpenGL context
     * и сделал его current.
     *
     * Передаём GLAD функцию, через которую он
     * сможет получать адреса OpenGL-функций
     * из текущего Qt context.
     */
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


    /*
     * Если OBJ был выбран раньше,
     * чем Qt успел создать OpenGL context,
     * загружаем Mesh только сейчас.
     */
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


    const Matrix4 projection =
        Projection::Perspective(
            45.0f,
            aspect,
            0.1f,
            100.0f
        );


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


    /*
     * Parsing выполняется на CPU.
     *
     * Mesh создаёт VAO/VBO/EBO,
     * поэтому его можно создавать только
     * при существующем current OpenGL context.
     */
    pending_mesh_data_ =
        std::move(mesh_data);


    if (gl_initialized_) {
        makeCurrent();

        UploadPendingMesh();

        doneCurrent();
    }


    content_label_->hide();

    update();
}


void SceneViewport::keyPressEvent(QKeyEvent* event)
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

        default:
            QOpenGLWidget::keyPressEvent(event);
            break;
    }
}


void SceneViewport::keyReleaseEvent(QKeyEvent* event)
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
            QOpenGLWidget::keyReleaseEvent(event);
            break;
    }
}

void SceneViewport::TickInput()
{
    constexpr float move_speed = 0.05f;

    if (move_forward_) {
        camera_.MoveForward(move_speed);
    }

    if (move_backward_) {
        camera_.MoveBackward(move_speed);
    }

    if (move_left_) {
        camera_.MoveLeft(move_speed);
    }

    if (move_right_) {
        camera_.MoveRight(move_speed);
    }

    if (move_up_) {
        camera_.MoveUp(move_speed);
    }

    if (move_down_) {
        camera_.MoveDown(move_speed);
    }
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


    /*
     * После загрузки в GPU текущему Renderer
     * CPU-копия render_vertices/render_indices
     * больше не требуется.
     */
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
        (minimum.x + maximum.x)
            * 0.5f,

        (minimum.y + maximum.y)
            * 0.5f,

        (minimum.z + maximum.z)
            * 0.5f
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