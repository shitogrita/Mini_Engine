#include "Editor/Panels/InspectorPanel.h"

#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QCheckBox>

#include <algorithm>
#include <utility>
#include <filesystem>

InspectorPanel::InspectorPanel(QWidget* parent)
    : QWidget(parent) {
    CreateLayout();
    ClearSelection();
}

void InspectorPanel::CreateLayout() {
    QVBoxLayout* main_layout = new QVBoxLayout(this);

    main_layout->setContentsMargins(10, 10, 10, 10);
    main_layout->setSpacing(10);

    title_label_ = new QLabel(this);

    title_label_->setStyleSheet(
        "font-size: 15px; font-weight: 600;"
    );

    QFrame* separator = new QFrame(this);

    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

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

    material_label_ = new QLabel("Material", this);

    material_label_->setStyleSheet(
        "font-weight: 600;"
    );

    QGridLayout* material_layout = new QGridLayout();

    material_layout->setHorizontalSpacing(6);
    material_layout->setVerticalSpacing(6);

    /*
     * PointLight.
     *
     * Источник света пока один на SceneViewport,
     * поэтому его настройки отображаются отдельным
     * блоком Inspector независимо от выбранного SceneObject.
     */
    light_label_ = new QLabel("Point Light", this);

    light_label_->setStyleSheet(
        "font-weight: 600;"
    );

    QGridLayout* light_layout = new QGridLayout();

    light_layout->setHorizontalSpacing(6);
    light_layout->setVerticalSpacing(6);

    light_position_x_ = CreateTransformSpinBox();
    light_position_y_ = CreateTransformSpinBox();
    light_position_z_ = CreateTransformSpinBox();

    light_intensity_ = CreateMaterialSpinBox(
        0.0,
        100.0,
        0.1
    );

    light_color_button_ = new QPushButton(
        "Select Color",
        this
    );

    light_enabled_ = new QCheckBox(
        "Enabled",
        this
    );

    light_layout->addWidget(new QLabel(""), 0, 0);
    light_layout->addWidget(new QLabel("X"), 0, 1);
    light_layout->addWidget(new QLabel("Y"), 0, 2);
    light_layout->addWidget(new QLabel("Z"), 0, 3);

    light_layout->addWidget(new QLabel("Position"), 1, 0);
    light_layout->addWidget(light_position_x_, 1, 1);
    light_layout->addWidget(light_position_y_, 1, 2);
    light_layout->addWidget(light_position_z_, 1, 3);

    light_layout->addWidget(new QLabel("Color"), 2, 0);
    light_layout->addWidget(light_color_button_, 2, 1, 1, 3);

    light_layout->addWidget(new QLabel("Intensity"), 3, 0);
    light_layout->addWidget(light_intensity_, 3, 1, 1, 3);

    light_layout->addWidget(new QLabel("State"), 4, 0);
    light_layout->addWidget(light_enabled_, 4, 1, 1, 3);

    material_combo_ = new QComboBox(this);

    material_color_button_ = new QPushButton("Select Color", this);
    material_texture_button_ = new QPushButton("Select Texture...", this);

    material_texture_name_ = new QLabel("None", this);
    material_texture_name_->setWordWrap(true);

    material_ambient_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_diffuse_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_specular_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_shininess_ = CreateMaterialSpinBox(1.0, 256.0, 1.0);

    material_layout->addWidget(new QLabel("Part"), 0, 0);
    material_layout->addWidget(material_combo_, 0, 1);

    material_layout->addWidget(new QLabel("Color"), 1, 0);
    material_layout->addWidget(material_color_button_, 1, 1);

    material_layout->addWidget(new QLabel("Texture"), 2, 0);
    material_layout->addWidget(material_texture_button_, 2, 1);

    material_layout->addWidget(new QLabel(""), 3, 0);
    material_layout->addWidget(material_texture_name_, 3, 1);

    material_layout->addWidget(new QLabel("Ambient"), 4, 0);
    material_layout->addWidget(material_ambient_, 4, 1);

    material_layout->addWidget(new QLabel("Diffuse"), 5, 0);
    material_layout->addWidget(material_diffuse_, 5, 1);

    material_layout->addWidget(new QLabel("Specular"), 6, 0);
    material_layout->addWidget(material_specular_, 6, 1);

    material_layout->addWidget(new QLabel("Shininess"), 7, 0);
    material_layout->addWidget(material_shininess_, 7, 1);

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

    main_layout->addWidget(light_label_);
    main_layout->addLayout(light_layout);

    main_layout->addWidget(information_label_);

    main_layout->addStretch();

    const auto update_transform = [this]() {
        if (updating_fields_ || !selected_object_) {
            return;
        }

        Transform& transform = selected_object_->GetTransform();

        transform.position.x = static_cast<float>(position_x_->value());
        transform.position.y = static_cast<float>(position_y_->value());
        transform.position.z = static_cast<float>(position_z_->value());

        transform.rotation.x = static_cast<float>(rotation_x_->value());
        transform.rotation.y = static_cast<float>(rotation_y_->value());
        transform.rotation.z = static_cast<float>(rotation_z_->value());

        transform.scale.x = static_cast<float>(scale_x_->value());
        transform.scale.y = static_cast<float>(scale_y_->value());
        transform.scale.z = static_cast<float>(scale_z_->value());

        if (transform_changed_callback_) {
            transform_changed_callback_();
        }
    };

    connect(
        position_x_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double) {
            update_transform();
        }
    );

    connect(
        position_y_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double) {
            update_transform();
        }
    );

    connect(
        position_z_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double) {
            update_transform();
        }
    );

    connect(
        rotation_x_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double) {
            update_transform();
        }
    );

    connect(
        rotation_y_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double) {
            update_transform();
        }
    );

    connect(
        rotation_z_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double) {
            update_transform();
        }
    );

    connect(
        scale_x_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double) {
            update_transform();
        }
    );

    connect(
        scale_y_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double) {
            update_transform();
        }
    );

    connect(
        scale_z_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_transform](double) {
            update_transform();
        }
    );

    connect(
        material_texture_button_,
        &QPushButton::clicked,
        this,
        [this]() {
            Material* material = GetSelectedMaterial();

            if (!material) {
                return;
            }

            const QString file_path = QFileDialog::getOpenFileName(
                this,
                "Select Texture",
                QString(),
                "Images (*.png *.jpg *.jpeg *.bmp)"
            );

            if (file_path.isEmpty()) {
                return;
            }

            if (texture_changed_callback_) {
                texture_changed_callback_(
                    *material,
                    file_path
                );
            }

            material_texture_name_->setText(
                QFileInfo(file_path).fileName()
            );

            if (transform_changed_callback_) {
                transform_changed_callback_();
            }
        }
    );

    const auto update_material = [this]() {
        if (updating_fields_) {
            return;
        }

        Material* material = GetSelectedMaterial();

        if (!material) {
            return;
        }

        material->SetAmbientStrength(
            static_cast<float>(material_ambient_->value())
        );

        material->SetDiffuseStrength(
            static_cast<float>(material_diffuse_->value())
        );

        material->SetSpecularStrength(
            static_cast<float>(material_specular_->value())
        );

        material->SetShininess(
            static_cast<float>(material_shininess_->value())
        );

        if (transform_changed_callback_) {
            transform_changed_callback_();
        }
    };

    connect(
        material_ambient_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_material](double) {
            update_material();
        }
    );

    connect(
        material_diffuse_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_material](double) {
            update_material();
        }
    );

    connect(
        material_specular_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_material](double) {
            update_material();
        }
    );

    connect(
        material_shininess_,
        &QDoubleSpinBox::valueChanged,
        this,
        [update_material](double) {
            update_material();
        }
    );

    connect(
        material_combo_,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            if (updating_fields_) {
                return;
            }

            UpdateMaterialFields();
        }
    );

    connect(
        material_color_button_,
        &QPushButton::clicked,
        this,
        [this]() {
            Material* material = GetSelectedMaterial();

            if (!material) {
                return;
            }

            const Vec3& current_color = material->GetColor();

            const QColor initial_color = QColor::fromRgbF(
                std::clamp(current_color.x, 0.0f, 1.0f),
                std::clamp(current_color.y, 0.0f, 1.0f),
                std::clamp(current_color.z, 0.0f, 1.0f)
            );

            const QColor selected_color = QColorDialog::getColor(
                initial_color,
                this,
                "Material Color"
            );

            if (!selected_color.isValid()) {
                return;
            }

            material->SetColor(
                Vec3{
                    static_cast<float>(selected_color.redF()),
                    static_cast<float>(selected_color.greenF()),
                    static_cast<float>(selected_color.blueF())
                }
            );

            UpdateMaterialFields();

            if (transform_changed_callback_) {
                transform_changed_callback_();
            }
        }
    );

    /**
     * Обновляет позицию PointLight из полей Inspector.
     */
    const auto update_light_position = [this]() {
        if (updating_fields_ || point_light_ == nullptr) {
            return;
        }

        point_light_->SetPosition(
            Vec3{
                static_cast<float>(light_position_x_->value()),
                static_cast<float>(light_position_y_->value()),
                static_cast<float>(light_position_z_->value())
            }
        );

        if (light_changed_callback_) {
            light_changed_callback_();
        }
    };

    connect(light_position_x_, &QDoubleSpinBox::valueChanged, this, [update_light_position](double) {
        update_light_position();
    });

    connect(light_position_y_, &QDoubleSpinBox::valueChanged, this, [update_light_position](double) {
        update_light_position();
    });

    connect(light_position_z_, &QDoubleSpinBox::valueChanged, this, [update_light_position](double) {
        update_light_position();
    });

    /**
     * Изменяет яркость PointLight.
     */
    connect(light_intensity_, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        if (updating_fields_ || point_light_ == nullptr) {
            return;
        }

        point_light_->SetIntensity(
            static_cast<float>(value)
        );

        if (light_changed_callback_) {
            light_changed_callback_();
        }
    });

    /**
     * Включает или выключает PointLight.
     */
    connect(light_enabled_, &QCheckBox::toggled, this, [this](bool enabled) {
        if (updating_fields_ || point_light_ == nullptr) {
            return;
        }

        point_light_->SetEnabled(enabled);

        if (light_changed_callback_) {
            light_changed_callback_();
        }
    });

    /**
     * Открывает стандартный Qt Color Picker
     * для изменения цвета PointLight.
     */
    connect(light_color_button_, &QPushButton::clicked, this, [this]() {
        if (point_light_ == nullptr) {
            return;
        }

        const Vec3& color =
            point_light_->GetColor();

        const QColor initial_color =
            QColor::fromRgbF(
                std::clamp(color.x, 0.0f, 1.0f),
                std::clamp(color.y, 0.0f, 1.0f),
                std::clamp(color.z, 0.0f, 1.0f)
            );

        const QColor selected_color =
            QColorDialog::getColor(
                initial_color,
                this,
                "Point Light Color"
            );

        if (!selected_color.isValid()) {
            return;
        }

        point_light_->SetColor(
            Vec3{
                static_cast<float>(selected_color.redF()),
                static_cast<float>(selected_color.greenF()),
                static_cast<float>(selected_color.blueF())
            }
        );

        UpdateLightColorButton();

        if (light_changed_callback_) {
            light_changed_callback_();
        }
    });
}

QDoubleSpinBox* InspectorPanel::CreateTransformSpinBox() {
    QDoubleSpinBox* spin_box = new QDoubleSpinBox(this);

    spin_box->setRange(-100000.0, 100000.0);
    spin_box->setDecimals(3);
    spin_box->setSingleStep(0.1);
    spin_box->setKeyboardTracking(true);

    return spin_box;
}

QDoubleSpinBox* InspectorPanel::CreateMaterialSpinBox(
    double minimum,
    double maximum,
    double step) {

    QDoubleSpinBox* spin_box = new QDoubleSpinBox(this);

    spin_box->setRange(minimum, maximum);
    spin_box->setDecimals(3);
    spin_box->setSingleStep(step);
    spin_box->setKeyboardTracking(true);

    return spin_box;
}

void InspectorPanel::SetObjectName(const QString& object_name) {
    title_label_->setText(object_name);

    transform_label_->show();
    material_label_->show();

    information_label_->setText(
        "Transform and Material properties of the selected SceneObject."
    );
}

void InspectorPanel::SetSelectedObject(std::shared_ptr<SceneObject> object) {
    selected_object_ = std::move(object);

    if (!selected_object_) {
        ClearSelection();
        return;
    }

    SetObjectName(
        QString::fromStdString(
            selected_object_->GetName()
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
    material_combo_->show();
    material_color_button_->show();

    material_ambient_->show();
    material_diffuse_->show();
    material_specular_->show();
    material_shininess_->show();

    material_texture_button_->show();
    material_texture_name_->show();

    UpdateTransformFields();
    UpdateMaterialList();
}

void InspectorPanel::RefreshTransformFields() {
    UpdateTransformFields();
}

void InspectorPanel::UpdateTransformFields() {
    if (!selected_object_) {
        return;
    }

    updating_fields_ = true;

    const Transform& transform = selected_object_->GetTransform();

    position_x_->setValue(transform.position.x);
    position_y_->setValue(transform.position.y);
    position_z_->setValue(transform.position.z);

    rotation_x_->setValue(transform.rotation.x);
    rotation_y_->setValue(transform.rotation.y);
    rotation_z_->setValue(transform.rotation.z);

    scale_x_->setValue(transform.scale.x);
    scale_y_->setValue(transform.scale.y);
    scale_z_->setValue(transform.scale.z);

    updating_fields_ = false;
}

void InspectorPanel::UpdateMaterialList() {
    updating_fields_ = true;

    material_combo_->clear();

    if (!selected_object_) {
        updating_fields_ = false;
        return;
    }

    if (selected_object_->HasRenderParts()) {
        const std::vector<SceneObject::SceneRenderPart>& parts =
            selected_object_->GetRenderParts();

        for (std::size_t i = 0; i < parts.size(); ++i) {
            const std::string& name = parts[i].name;

            if (name.empty()) {
                material_combo_->addItem(
                    QString("Material %1").arg(i + 1)
                );
            } else {
                material_combo_->addItem(
                    QString::fromStdString(name)
                );
            }
        }
    } else {
        material_combo_->addItem("Material");
    }

    if (material_combo_->count() > 0) {
        material_combo_->setCurrentIndex(0);
    }

    updating_fields_ = false;

    UpdateMaterialFields();
}

Material* InspectorPanel::GetSelectedMaterial() {
    if (!selected_object_) {
        return nullptr;
    }

    if (!selected_object_->HasRenderParts()) {
        return &selected_object_->GetMaterial();
    }

    std::vector<SceneObject::SceneRenderPart>& parts =
        selected_object_->GetRenderParts();

    const int index = material_combo_->currentIndex();

    if (index < 0) {
        return nullptr;
    }

    if (static_cast<std::size_t>(index) >= parts.size()) {
        return nullptr;
    }

    return &parts[static_cast<std::size_t>(index)].material;
}

void InspectorPanel::UpdateMaterialFields() {
    Material* material = GetSelectedMaterial();

    if (!material) {
        return;
    }

    updating_fields_ = true;

    material_ambient_->setValue(
        material->GetAmbientStrength()
    );

    material_diffuse_->setValue(
        material->GetDiffuseStrength()
    );

    material_specular_->setValue(
        material->GetSpecularStrength()
    );

    material_shininess_->setValue(
        material->GetShininess()
    );

    const Vec3& color = material->GetColor();

    const QColor q_color = QColor::fromRgbF(
        std::clamp(color.x, 0.0f, 1.0f),
        std::clamp(color.y, 0.0f, 1.0f),
        std::clamp(color.z, 0.0f, 1.0f)
    );

    material_color_button_->setStyleSheet(
        QString(
            "background-color: %1;"
            "border: 1px solid #666;"
            "min-height: 22px;"
        ).arg(q_color.name())
    );

    updating_fields_ = false;
    const std::filesystem::path& texture_path = material->GetDiffuseTexturePath();

    if (texture_path.empty()) {
        material_texture_name_->setText("None");
    } else {
        material_texture_name_->setText(
            QString::fromStdString(texture_path.filename().string())
        );
    }
}

void InspectorPanel::SetTransformChangedCallback(
    std::function<void()> callback) {

    transform_changed_callback_ = std::move(callback);
}

void InspectorPanel::ClearSelection() {
    selected_object_.reset();

    title_label_->setText("No object selected");

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
    material_combo_->hide();
    material_color_button_->hide();

    material_ambient_->hide();
    material_diffuse_->hide();
    material_specular_->hide();
    material_shininess_->hide();

    material_texture_button_->hide();
    material_texture_name_->hide();

    material_texture_name_->setText("None");

    material_combo_->clear();

    information_label_->setText(
        "Select an object in Hierarchy to inspect it."
    );
}

void InspectorPanel::SetTextureChangedCallback(
    std::function<void(Material&, const QString&)> callback) {

    texture_changed_callback_ = std::move(callback);
}

/**
 * @brief Подключает PointLight к Inspector.
 *
 * Inspector не владеет источником света.
 * PointLight продолжает принадлежать SceneViewport.
 *
 * После подключения текущие параметры света
 * сразу отображаются в пользовательском интерфейсе.
 *
 * @param point_light Указатель на PointLight сцены.
 */
void InspectorPanel::SetPointLight(PointLight* point_light) {
    point_light_ = point_light;
    UpdateLightFields();
}

/**
 * @brief Устанавливает callback изменения PointLight.
 *
 * Callback вызывается после изменения:
 * - позиции;
 * - цвета;
 * - интенсивности;
 * - состояния Enabled.
 *
 * EditorWindow использует его для немедленной
 * перерисовки SceneViewport.
 *
 * @param callback Функция, вызываемая после изменения света.
 */
void InspectorPanel::SetLightChangedCallback(std::function<void()> callback) {
    light_changed_callback_ = std::move(callback);
}

/**
 * @brief Обновляет элементы Inspector
 * текущими параметрами PointLight.
 *
 * Метод используется после подключения источника света
 * и может использоваться в дальнейшем после изменения
 * PointLight через Gizmo.
 */
void InspectorPanel::UpdateLightFields() {
    if (point_light_ == nullptr) {
        return;
    }

    updating_fields_ = true;

    const Vec3& position = point_light_->GetPosition();

    light_position_x_->setValue(position.x);
    light_position_y_->setValue(position.y);
    light_position_z_->setValue(position.z);

    light_intensity_->setValue(point_light_->GetIntensity());
    light_enabled_->setChecked(point_light_->IsEnabled());

    UpdateLightColorButton();

    updating_fields_ = false;
}

/**
 * @brief Обновляет внешний вид кнопки выбора цвета света.
 *
 * Цвет фона кнопки соответствует текущему RGB-цвету
 * PointLight, благодаря чему цвет источника виден
 * непосредственно в Inspector.
 */
void InspectorPanel::UpdateLightColorButton() {
    if (point_light_ == nullptr || light_color_button_ == nullptr) {
        return;
    }

    const Vec3& color = point_light_->GetColor();

    const QColor q_color = QColor::fromRgbF(
        std::clamp(color.x, 0.0f, 1.0f),
        std::clamp(color.y, 0.0f, 1.0f),
        std::clamp(color.z, 0.0f, 1.0f)
    );

    light_color_button_->setStyleSheet(
        QString(
            "background-color: %1;"
            "border: 1px solid #666;"
            "min-height: 22px;"
        ).arg(q_color.name())
    );
}