#include "Editor/Panels/InspectorPanel.h"

#include <QColor>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>


InspectorPanel::InspectorPanel(QWidget* parent)
    : QWidget(parent)
{
    CreateLayout();
    ClearSelection();
}


void InspectorPanel::CreateLayout()
{
    QVBoxLayout* main_layout = new QVBoxLayout(this);

    main_layout->setContentsMargins(10, 10, 10, 10);
    main_layout->setSpacing(10);


    /*
     * Заголовок выбранного SceneObject.
     */
    title_label_ = new QLabel(this);

    title_label_->setStyleSheet(
        "font-size: 15px; font-weight: 600;"
    );


    QFrame* separator = new QFrame(this);

    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);


    /*
     * Transform.
     */
    transform_label_ = new QLabel("Transform", this);

    transform_label_->setStyleSheet(
        "font-weight: 600;"
    );


    QGridLayout* transform_layout = new QGridLayout();

    transform_layout->setHorizontalSpacing(6);
    transform_layout->setVerticalSpacing(6);

    transform_layout->addWidget(new QLabel(""), 0, 0);
    transform_layout->addWidget(new QLabel("X"), 0, 1);
    transform_layout->addWidget(new QLabel("Y"), 0, 2);
    transform_layout->addWidget(new QLabel("Z"), 0, 3);


    position_x_ = CreateTransformSpinBox();
    position_y_ = CreateTransformSpinBox();
    position_z_ = CreateTransformSpinBox();

    rotation_x_ = CreateTransformSpinBox();
    rotation_y_ = CreateTransformSpinBox();
    rotation_z_ = CreateTransformSpinBox();

    scale_x_ = CreateTransformSpinBox();
    scale_y_ = CreateTransformSpinBox();
    scale_z_ = CreateTransformSpinBox();


    transform_layout->addWidget(new QLabel("Position"), 1, 0);
    transform_layout->addWidget(position_x_, 1, 1);
    transform_layout->addWidget(position_y_, 1, 2);
    transform_layout->addWidget(position_z_, 1, 3);

    transform_layout->addWidget(new QLabel("Rotation"), 2, 0);
    transform_layout->addWidget(rotation_x_, 2, 1);
    transform_layout->addWidget(rotation_y_, 2, 2);
    transform_layout->addWidget(rotation_z_, 2, 3);

    transform_layout->addWidget(new QLabel("Scale"), 3, 0);
    transform_layout->addWidget(scale_x_, 3, 1);
    transform_layout->addWidget(scale_y_, 3, 2);
    transform_layout->addWidget(scale_z_, 3, 3);


    /*
     * Material.
     *
     * Material относится непосредственно
     * к выбранному SceneObject.
     */
    material_label_ = new QLabel("Material", this);

    material_label_->setStyleSheet(
        "font-weight: 600;"
    );


    QGridLayout* material_layout = new QGridLayout();

    material_layout->setHorizontalSpacing(6);
    material_layout->setVerticalSpacing(6);


    material_color_button_ = new QPushButton("Select Color", this);

    material_ambient_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_diffuse_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_specular_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_shininess_ = CreateMaterialSpinBox(1.0, 256.0, 1.0);


    material_layout->addWidget(
        new QLabel("Color"),
        0,
        0
    );

    material_layout->addWidget(
        material_color_button_,
        0,
        1
    );


    material_layout->addWidget(
        new QLabel("Ambient"),
        1,
        0
    );

    material_layout->addWidget(
        material_ambient_,
        1,
        1
    );


    material_layout->addWidget(
        new QLabel("Diffuse"),
        2,
        0
    );

    material_layout->addWidget(
        material_diffuse_,
        2,
        1
    );


    material_layout->addWidget(
        new QLabel("Specular"),
        3,
        0
    );

    material_layout->addWidget(
        material_specular_,
        3,
        1
    );


    material_layout->addWidget(
        new QLabel("Shininess"),
        4,
        0
    );

    material_layout->addWidget(
        material_shininess_,
        4,
        1
    );


    information_label_ = new QLabel(this);

    information_label_->setAlignment(
        Qt::AlignTop |
        Qt::AlignLeft
    );

    information_label_->setWordWrap(true);


    main_layout->addWidget(title_label_);
    main_layout->addWidget(separator);

    main_layout->addWidget(transform_label_);
    main_layout->addLayout(transform_layout);

    main_layout->addWidget(material_label_);
    main_layout->addLayout(material_layout);

    main_layout->addWidget(information_label_);

    main_layout->addStretch();


    /*
     * Общая функция изменения Transform.
     */
    const auto update_transform =
        [this]()
        {
            if (
                updating_fields_ ||
                !selected_object_
            ) {
                return;
            }

            Transform& transform =
                selected_object_->
                    GetTransform();


            transform.position.x =
                static_cast<float>(
                    position_x_->value()
                );

            transform.position.y =
                static_cast<float>(
                    position_y_->value()
                );

            transform.position.z =
                static_cast<float>(
                    position_z_->value()
                );


            transform.rotation.x =
                static_cast<float>(
                    rotation_x_->value()
                );

            transform.rotation.y =
                static_cast<float>(
                    rotation_y_->value()
                );

            transform.rotation.z =
                static_cast<float>(
                    rotation_z_->value()
                );


            transform.scale.x =
                static_cast<float>(
                    scale_x_->value()
                );

            transform.scale.y =
                static_cast<float>(
                    scale_y_->value()
                );

            transform.scale.z =
                static_cast<float>(
                    scale_z_->value()
                );


            if (transform_changed_callback_) {
                transform_changed_callback_();
            }
        };


    connect(
        position_x_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double)
        {
            update_transform();
        }
    );

    connect(
        position_y_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double)
        {
            update_transform();
        }
    );

    connect(
        position_z_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double)
        {
            update_transform();
        }
    );


    connect(
        rotation_x_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double)
        {
            update_transform();
        }
    );

    connect(
        rotation_y_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double)
        {
            update_transform();
        }
    );

    connect(
        rotation_z_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double)
        {
            update_transform();
        }
    );


    connect(
        scale_x_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double)
        {
            update_transform();
        }
    );

    connect(
        scale_y_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double)
        {
            update_transform();
        }
    );

    connect(
        scale_z_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double)
        {
            update_transform();
        }
    );


    /*
     * Общая функция изменения числовых
     * параметров Material.
     */
    const auto update_material =
        [this]()
        {
            if (
                updating_fields_ ||
                !selected_object_
            ) {
                return;
            }


            Material& material =
                selected_object_->
                    GetMaterial();


            material.SetAmbientStrength(
                static_cast<float>(
                    material_ambient_->value()
                )
            );

            material.SetDiffuseStrength(
                static_cast<float>(
                    material_diffuse_->value()
                )
            );

            material.SetSpecularStrength(
                static_cast<float>(
                    material_specular_->value()
                )
            );

            material.SetShininess(
                static_cast<float>(
                    material_shininess_->value()
                )
            );


            /*
             * Пока используем существующий callback:
             * его задача — сообщить EditorWindow,
             * что SceneObject изменился и viewport
             * необходимо перерисовать.
             */
            if (transform_changed_callback_) {
                transform_changed_callback_();
            }
        };


    connect(
        material_ambient_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_material](double)
        {
            update_material();
        }
    );

    connect(
        material_diffuse_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_material](double)
        {
            update_material();
        }
    );

    connect(
        material_specular_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_material](double)
        {
            update_material();
        }
    );

    connect(
        material_shininess_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_material](double)
        {
            update_material();
        }
    );


    /*
     * Выбор базового цвета Material.
     */
    connect(
        material_color_button_,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (!selected_object_) {
                return;
            }


            const Vec3& current_color =
                selected_object_->
                    GetMaterial().
                    GetColor();


            const QColor initial_color =
                QColor::fromRgbF(
                    std::clamp(
                        current_color.x,
                        0.0f,
                        1.0f
                    ),
                    std::clamp(
                        current_color.y,
                        0.0f,
                        1.0f
                    ),
                    std::clamp(
                        current_color.z,
                        0.0f,
                        1.0f
                    )
                );


            const QColor selected_color =
                QColorDialog::getColor(
                    initial_color,
                    this,
                    "Material Color"
                );


            if (!selected_color.isValid()) {
                return;
            }


            selected_object_->
                GetMaterial().
                SetColor(
                    Vec3{
                        static_cast<float>(
                            selected_color.redF()
                        ),
                        static_cast<float>(
                            selected_color.greenF()
                        ),
                        static_cast<float>(
                            selected_color.blueF()
                        )
                    }
                );


            UpdateMaterialFields();


            if (transform_changed_callback_) {
                transform_changed_callback_();
            }
        }
    );
}


QDoubleSpinBox*
InspectorPanel::CreateTransformSpinBox()
{
    QDoubleSpinBox* spin_box =
        new QDoubleSpinBox(this);

    spin_box->setRange(
        -100000.0,
        100000.0
    );

    spin_box->setDecimals(3);

    spin_box->setSingleStep(
        0.1
    );

    spin_box->setKeyboardTracking(
        true
    );

    return spin_box;
}


QDoubleSpinBox*
InspectorPanel::CreateMaterialSpinBox(
    double minimum,
    double maximum,
    double step
)
{
    QDoubleSpinBox* spin_box =
        new QDoubleSpinBox(this);

    spin_box->setRange(
        minimum,
        maximum
    );

    spin_box->setDecimals(3);

    spin_box->setSingleStep(
        step
    );

    spin_box->setKeyboardTracking(
        true
    );

    return spin_box;
}


void InspectorPanel::SetObjectName(
    const QString& object_name
)
{
    title_label_->setText(
        object_name
    );

    transform_label_->show();
    material_label_->show();

    information_label_->setText(
        "Transform and Material properties of the selected SceneObject."
    );
}


void InspectorPanel::SetSelectedObject(
    std::shared_ptr<SceneObject> object
)
{
    selected_object_ =
        std::move(object);


    if (!selected_object_) {
        ClearSelection();
        return;
    }


    SetObjectName(
        QString::fromStdString(
            selected_object_->
                GetName()
        )
    );


    transform_label_->show();

    position_x_->show();
    position_y_->show();
    position_z_->show();

    rotation_x_->show();
    rotation_y_->show();
    rotation_z_->show();

    scale_x_->show();
    scale_y_->show();
    scale_z_->show();


    material_label_->show();

    material_color_button_->show();

    material_ambient_->show();
    material_diffuse_->show();
    material_specular_->show();
    material_shininess_->show();


    UpdateTransformFields();
    UpdateMaterialFields();
}


void InspectorPanel::RefreshTransformFields()
{
    /*
     * Transform мог измениться за пределами Inspector,
     * например во время перемещения Move Gizmo.
     */
    UpdateTransformFields();
}


void InspectorPanel::UpdateTransformFields()
{
    if (!selected_object_) {
        return;
    }


    /*
     * setValue() генерирует valueChanged.
     *
     * Поэтому на время программного обновления
     * блокируем нашу логику изменения объекта.
     */
    updating_fields_ =
        true;


    const Transform& transform =
        selected_object_->
            GetTransform();


    position_x_->setValue(
        transform.position.x
    );

    position_y_->setValue(
        transform.position.y
    );

    position_z_->setValue(
        transform.position.z
    );


    rotation_x_->setValue(
        transform.rotation.x
    );

    rotation_y_->setValue(
        transform.rotation.y
    );

    rotation_z_->setValue(
        transform.rotation.z
    );


    scale_x_->setValue(
        transform.scale.x
    );

    scale_y_->setValue(
        transform.scale.y
    );

    scale_z_->setValue(
        transform.scale.z
    );


    updating_fields_ =
        false;
}


void InspectorPanel::UpdateMaterialFields()
{
    if (!selected_object_) {
        return;
    }


    updating_fields_ =
        true;


    const Material& material =
        selected_object_->
            GetMaterial();


    material_ambient_->setValue(
        material.GetAmbientStrength()
    );

    material_diffuse_->setValue(
        material.GetDiffuseStrength()
    );

    material_specular_->setValue(
        material.GetSpecularStrength()
    );

    material_shininess_->setValue(
        material.GetShininess()
    );


    const Vec3& color =
        material.GetColor();


    const QColor q_color =
        QColor::fromRgbF(
            std::clamp(
                color.x,
                0.0f,
                1.0f
            ),
            std::clamp(
                color.y,
                0.0f,
                1.0f
            ),
            std::clamp(
                color.z,
                0.0f,
                1.0f
            )
        );


    material_color_button_->setStyleSheet(
        QString(
            "background-color: %1;"
            "border: 1px solid #666;"
            "min-height: 22px;"
        )
        .arg(
            q_color.name()
        )
    );


    updating_fields_ =
        false;
}


void InspectorPanel::SetTransformChangedCallback(
    std::function<void()> callback
)
{
    transform_changed_callback_ =
        std::move(callback);
}


void InspectorPanel::ClearSelection()
{
    selected_object_.reset();


    title_label_->setText(
        "No object selected"
    );


    transform_label_->hide();

    position_x_->hide();
    position_y_->hide();
    position_z_->hide();

    rotation_x_->hide();
    rotation_y_->hide();
    rotation_z_->hide();

    scale_x_->hide();
    scale_y_->hide();
    scale_z_->hide();


    material_label_->hide();

    material_color_button_->hide();

    material_ambient_->hide();
    material_diffuse_->hide();
    material_specular_->hide();
    material_shininess_->hide();


    information_label_->setText(
        "Select an object in Hierarchy to inspect it."
    );
}