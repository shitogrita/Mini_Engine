#include "Editor/Panels/InspectorPanel.h"

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include <algorithm>
#include <filesystem>
#include <utility>

/**
 * @brief Создаёт Inspector.
 *
 * При запуске Inspector не имеет выбранного объекта,
 * поэтому все компонентные секции скрываются.
 *
 * @param parent Родительский QWidget.
 */
InspectorPanel::InspectorPanel(QWidget* parent) : QWidget(parent) {
    CreateLayout();
    ClearSelection();
}

/**
 * @brief Создаёт полный интерфейс Inspector.
 *
 * Transform является общей частью любого SceneObject.
 *
 * Material отображается только для обычной геометрии.
 *
 * Light отображается только для Point Light.
 */
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

    /*
     * Transform.
     *
     * Transform используется любым SceneObject,
     * включая Point Light.
     */
    transform_section_ = new QWidget(this);

    QVBoxLayout* transform_section_layout = new QVBoxLayout(transform_section_);

    transform_section_layout->setContentsMargins(0, 0, 0, 0);
    transform_section_layout->setSpacing(6);

    transform_label_ = new QLabel("Transform", transform_section_);

    transform_label_->setStyleSheet(
        "font-weight: 600;"
    );

    QGridLayout* transform_layout = new QGridLayout();

    transform_layout->setHorizontalSpacing(6);
    transform_layout->setVerticalSpacing(6);

    transform_layout->addWidget(new QLabel("", transform_section_), 0, 0);
    transform_layout->addWidget(new QLabel("X", transform_section_), 0, 1);
    transform_layout->addWidget(new QLabel("Y", transform_section_), 0, 2);
    transform_layout->addWidget(new QLabel("Z", transform_section_), 0, 3);

    position_x_ = CreateTransformSpinBox();
    position_y_ = CreateTransformSpinBox();
    position_z_ = CreateTransformSpinBox();

    rotation_x_ = CreateTransformSpinBox();
    rotation_y_ = CreateTransformSpinBox();
    rotation_z_ = CreateTransformSpinBox();

    scale_x_ = CreateTransformSpinBox();
    scale_y_ = CreateTransformSpinBox();
    scale_z_ = CreateTransformSpinBox();

    transform_layout->addWidget(new QLabel("Position", transform_section_), 1, 0);
    transform_layout->addWidget(position_x_, 1, 1);
    transform_layout->addWidget(position_y_, 1, 2);
    transform_layout->addWidget(position_z_, 1, 3);

    transform_layout->addWidget(new QLabel("Rotation", transform_section_), 2, 0);
    transform_layout->addWidget(rotation_x_, 2, 1);
    transform_layout->addWidget(rotation_y_, 2, 2);
    transform_layout->addWidget(rotation_z_, 2, 3);

    transform_layout->addWidget(new QLabel("Scale", transform_section_), 3, 0);
    transform_layout->addWidget(scale_x_, 3, 1);
    transform_layout->addWidget(scale_y_, 3, 2);
    transform_layout->addWidget(scale_z_, 3, 3);

    transform_section_layout->addWidget(transform_label_);
    transform_section_layout->addLayout(transform_layout);

    /*
     * Material.
     *
     * Весь Material находится в отдельном QWidget,
     * поэтому его можно полностью скрыть
     * при выборе Point Light.
     */
    material_section_ = new QWidget(this);

    QVBoxLayout* material_section_layout = new QVBoxLayout(material_section_);

    material_section_layout->setContentsMargins(0, 0, 0, 0);
    material_section_layout->setSpacing(6);

    material_label_ = new QLabel("Material", material_section_);

    material_label_->setStyleSheet(
        "font-weight: 600;"
    );

    QGridLayout* material_layout = new QGridLayout();

    material_layout->setHorizontalSpacing(6);
    material_layout->setVerticalSpacing(6);

    material_combo_ = new QComboBox(material_section_);

    material_color_button_ = new QPushButton(
        "Select Color",
        material_section_
    );

    material_texture_button_ = new QPushButton(
        "Select Texture...",
        material_section_
    );

    material_texture_name_ = new QLabel(
        "None",
        material_section_
    );

    material_texture_name_->setWordWrap(true);

    material_ambient_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_diffuse_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_specular_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_shininess_ = CreateMaterialSpinBox(1.0, 256.0, 1.0);

    material_layout->addWidget(new QLabel("Part", material_section_), 0, 0);
    material_layout->addWidget(material_combo_, 0, 1);

    material_layout->addWidget(new QLabel("Color", material_section_), 1, 0);
    material_layout->addWidget(material_color_button_, 1, 1);

    material_layout->addWidget(new QLabel("Texture", material_section_), 2, 0);
    material_layout->addWidget(material_texture_button_, 2, 1);

    material_layout->addWidget(new QLabel("", material_section_), 3, 0);
    material_layout->addWidget(material_texture_name_, 3, 1);

    material_layout->addWidget(new QLabel("Ambient", material_section_), 4, 0);
    material_layout->addWidget(material_ambient_, 4, 1);

    material_layout->addWidget(new QLabel("Diffuse", material_section_), 5, 0);
    material_layout->addWidget(material_diffuse_, 5, 1);

    material_layout->addWidget(new QLabel("Specular", material_section_), 6, 0);
    material_layout->addWidget(material_specular_, 6, 1);

    material_layout->addWidget(new QLabel("Shininess", material_section_), 7, 0);
    material_layout->addWidget(material_shininess_, 7, 1);

    material_section_layout->addWidget(material_label_);
    material_section_layout->addLayout(material_layout);

    /*
     * Light.
     *
     * Здесь находятся только свойства компонента света.
     *
     * Position отсутствует специально:
     * Point Light перемещается через обычный Transform
     * своего SceneObject.
     */
    light_section_ = new QWidget(this);

    QVBoxLayout* light_section_layout = new QVBoxLayout(light_section_);

    light_section_layout->setContentsMargins(0, 0, 0, 0);
    light_section_layout->setSpacing(6);

    light_label_ = new QLabel(
        "Light",
        light_section_
    );

    light_label_->setStyleSheet(
        "font-weight: 600;"
    );

    light_color_button_ = new QPushButton(
        "Select Color",
        light_section_
    );

    light_intensity_ = CreateMaterialSpinBox(
        0.0,
        100.0,
        0.1
    );

    light_enabled_ = new QCheckBox(
        "Enabled",
        light_section_
    );

    QGridLayout* light_layout = new QGridLayout();

    light_layout->setHorizontalSpacing(6);
    light_layout->setVerticalSpacing(6);

    light_layout->addWidget(new QLabel("Color", light_section_), 0, 0);
    light_layout->addWidget(light_color_button_, 0, 1);

    light_layout->addWidget(new QLabel("Intensity", light_section_), 1, 0);
    light_layout->addWidget(light_intensity_, 1, 1);

    light_layout->addWidget(new QLabel("State", light_section_), 2, 0);
    light_layout->addWidget(light_enabled_, 2, 1);

    light_section_layout->addWidget(light_label_);
    light_section_layout->addLayout(light_layout);

    /*
     * Информационный текст.
     */
    information_label_ = new QLabel(this);

    information_label_->setAlignment(
        Qt::AlignTop |
        Qt::AlignLeft
    );

    information_label_->setWordWrap(true);

    /*
     * Финальная структура Inspector.
     */
    main_layout->addWidget(title_label_);
    main_layout->addWidget(separator);
    main_layout->addWidget(transform_section_);
    main_layout->addWidget(material_section_);
    main_layout->addWidget(light_section_);
    main_layout->addWidget(information_label_);
    main_layout->addStretch();

    /*
     * Transform callback.
     *
     * Один callback обслуживает все девять полей,
     * потому что после любого изменения мы просто
     * переносим полное состояние UI в Transform.
     */
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

    connect(position_x_, &QDoubleSpinBox::valueChanged, this, [update_transform](double) {
        update_transform();
    });

    connect(position_y_, &QDoubleSpinBox::valueChanged, this, [update_transform](double) {
        update_transform();
    });

    connect(position_z_, &QDoubleSpinBox::valueChanged, this, [update_transform](double) {
        update_transform();
    });

    connect(rotation_x_, &QDoubleSpinBox::valueChanged, this, [update_transform](double) {
        update_transform();
    });

    connect(rotation_y_, &QDoubleSpinBox::valueChanged, this, [update_transform](double) {
        update_transform();
    });

    connect(rotation_z_, &QDoubleSpinBox::valueChanged, this, [update_transform](double) {
        update_transform();
    });

    connect(scale_x_, &QDoubleSpinBox::valueChanged, this, [update_transform](double) {
        update_transform();
    });

    connect(scale_y_, &QDoubleSpinBox::valueChanged, this, [update_transform](double) {
        update_transform();
    });

    connect(scale_z_, &QDoubleSpinBox::valueChanged, this, [update_transform](double) {
        update_transform();
    });

    /*
     * Выбор RenderPart multipart-модели.
     */
    connect(material_combo_, &QComboBox::currentIndexChanged, this, [this](int) {
        if (updating_fields_) {
            return;
        }

        UpdateMaterialFields();
    });

    /*
     * Изменение числовых параметров Material.
     */
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

    connect(material_ambient_, &QDoubleSpinBox::valueChanged, this, [update_material](double) {
        update_material();
    });

    connect(material_diffuse_, &QDoubleSpinBox::valueChanged, this, [update_material](double) {
        update_material();
    });

    connect(material_specular_, &QDoubleSpinBox::valueChanged, this, [update_material](double) {
        update_material();
    });

    connect(material_shininess_, &QDoubleSpinBox::valueChanged, this, [update_material](double) {
        update_material();
    });

    /*
     * Изменение базового цвета Material.
     */
    connect(material_color_button_, &QPushButton::clicked, this, [this]() {
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
    });

    /*
     * Выбор Texture для Material.
     *
     * Загрузка передаётся наружу,
     * потому что Texture2D требует активный OpenGL Context.
     */
    connect(material_texture_button_, &QPushButton::clicked, this, [this]() {
        Material* material = GetSelectedMaterial();

        if (!material) {
            return;
        }

        const QString file_path = QFileDialog::getOpenFileName(
            this,
            "Select Texture",
            QString(),
            "Images (*.png *.jpg *.jpeg *.bmp);;All files (*.*)"
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
    });

    /*
     * Изменение интенсивности Point Light.
     *
     * Callback работает только если выбран
     * непосредственно SceneObject источника света.
     */
    connect(light_intensity_, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        if (
            updating_fields_ ||
            !selected_object_ ||
            selected_object_->GetType() != SceneObject::Type::PointLight ||
            point_light_ == nullptr
        ) {
            return;
        }

        point_light_->SetIntensity(
            static_cast<float>(value)
        );

        if (light_changed_callback_) {
            light_changed_callback_();
        }
    });

    /*
     * Enabled Point Light.
     */
    connect(light_enabled_, &QCheckBox::toggled, this, [this](bool enabled) {
        if (
            updating_fields_ ||
            !selected_object_ ||
            selected_object_->GetType() != SceneObject::Type::PointLight ||
            point_light_ == nullptr
        ) {
            return;
        }

        point_light_->SetEnabled(enabled);

        if (light_changed_callback_) {
            light_changed_callback_();
        }
    });

    /*
     * Изменение цвета Point Light.
     */
    connect(light_color_button_, &QPushButton::clicked, this, [this]() {
        if (
            !selected_object_ ||
            selected_object_->GetType() != SceneObject::Type::PointLight ||
            point_light_ == nullptr
        ) {
            return;
        }

        const Vec3& color = point_light_->GetColor();

        const QColor initial_color = QColor::fromRgbF(
            std::clamp(color.x, 0.0f, 1.0f),
            std::clamp(color.y, 0.0f, 1.0f),
            std::clamp(color.z, 0.0f, 1.0f)
        );

        const QColor selected_color = QColorDialog::getColor(
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

    /*
     * До первого selection компонентные секции
     * Inspector не отображаются.
     */
    transform_section_->hide();
    material_section_->hide();
    light_section_->hide();
}

/**
 * @brief Создаёт числовое поле Transform.
 *
 * @return Настроенный QDoubleSpinBox.
 */
QDoubleSpinBox* InspectorPanel::CreateTransformSpinBox() {
    QDoubleSpinBox* spin_box = new QDoubleSpinBox(this);

    spin_box->setRange(-100000.0, 100000.0);
    spin_box->setDecimals(3);
    spin_box->setSingleStep(0.1);
    spin_box->setKeyboardTracking(true);

    return spin_box;
}

/**
 * @brief Создаёт числовое поле Material или Light.
 *
 * @param minimum Минимальное значение.
 * @param maximum Максимальное значение.
 * @param step Шаг изменения.
 *
 * @return Настроенный QDoubleSpinBox.
 */
QDoubleSpinBox* InspectorPanel::CreateMaterialSpinBox(double minimum, double maximum, double step) {
    QDoubleSpinBox* spin_box = new QDoubleSpinBox(this);

    spin_box->setRange(minimum, maximum);
    spin_box->setDecimals(3);
    spin_box->setSingleStep(step);
    spin_box->setKeyboardTracking(true);

    return spin_box;
}

/**
 * @brief Устанавливает имя выбранного объекта.
 *
 * Метод не решает, какие компоненты нужно показывать.
 * Это делает SetSelectedObject().
 *
 * @param object_name Имя SceneObject.
 */
void InspectorPanel::SetObjectName(const QString& object_name) {
    title_label_->setText(object_name);
}

/**
 * @brief Устанавливает текущий SceneObject Inspector.
 *
 * Для обычного SceneObject отображается:
 *
 * Transform + Material.
 *
 * Для Point Light отображается:
 *
 * Transform + Light.
 *
 * @param object Новый выбранный объект.
 */
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

    /*
     * Transform существует у любого SceneObject.
     */
    transform_section_->show();

    UpdateTransformFields();

    const bool is_point_light =
        selected_object_->GetType() ==
        SceneObject::Type::PointLight;

    /*
     * Point Light:
     *
     * Transform + Light.
     *
     * Material полностью скрывается.
     */
    if (is_point_light) {
        material_section_->hide();
        light_section_->show();

        UpdateLightFields();

        information_label_->setText(
            "Point Light properties."
        );

        return;
    }

    /*
     * Обычный объект:
     *
     * Transform + Material.
     *
     * Light полностью скрывается.
     */
    light_section_->hide();
    material_section_->show();

    UpdateMaterialList();

    information_label_->setText(
        "Transform and Material properties of the selected SceneObject."
    );
}

/**
 * @brief Обновляет Transform после изменения через Gizmo.
 */
void InspectorPanel::RefreshTransformFields() {
    UpdateTransformFields();
}

/**
 * @brief Переносит Transform выбранного объекта в UI.
 */
void InspectorPanel::UpdateTransformFields() {
    if (!selected_object_) {
        return;
    }

    updating_fields_ = true;

    const Transform& transform =
        selected_object_->GetTransform();

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

/**
 * @brief Создаёт список материалов выбранного объекта.
 *
 * Multipart OBJ отображает все RenderPart.
 */
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

        for (std::size_t index = 0; index < parts.size(); ++index) {
            const std::string& name =
                parts[index].name;

            if (name.empty()) {
                material_combo_->addItem(
                    QString("Material %1").arg(index + 1)
                );
            } else {
                material_combo_->addItem(
                    QString::fromStdString(name)
                );
            }
        }
    } else {
        material_combo_->addItem(
            "Material"
        );
    }

    if (material_combo_->count() > 0) {
        material_combo_->setCurrentIndex(0);
    }

    updating_fields_ = false;

    UpdateMaterialFields();
}

/**
 * @brief Возвращает текущий Material Inspector.
 *
 * @return Material или nullptr.
 */
Material* InspectorPanel::GetSelectedMaterial() {
    if (!selected_object_) {
        return nullptr;
    }

    /*
     * Point Light не имеет редактируемого Material.
     */
    if (
        selected_object_->GetType() ==
        SceneObject::Type::PointLight
    ) {
        return nullptr;
    }

    if (!selected_object_->HasRenderParts()) {
        return &selected_object_->GetMaterial();
    }

    std::vector<SceneObject::SceneRenderPart>& parts =
        selected_object_->GetRenderParts();

    const int index =
        material_combo_->currentIndex();

    if (index < 0) {
        return nullptr;
    }

    if (
        static_cast<std::size_t>(index) >=
        parts.size()
    ) {
        return nullptr;
    }

    return &parts[
        static_cast<std::size_t>(index)
    ].material;
}

/**
 * @brief Обновляет поля выбранного Material.
 */
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

    const Vec3& color =
        material->GetColor();

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

    const std::filesystem::path& texture_path =
        material->GetDiffuseTexturePath();

    if (texture_path.empty()) {
        material_texture_name_->setText(
            "None"
        );
    } else {
        material_texture_name_->setText(
            QString::fromStdString(
                texture_path.filename().string()
            )
        );
    }

    updating_fields_ = false;
}

/**
 * @brief Устанавливает callback изменения SceneObject.
 *
 * @param callback Функция обновления viewport.
 */
void InspectorPanel::SetTransformChangedCallback(std::function<void()> callback) {
    transform_changed_callback_ =
        std::move(callback);
}

/**
 * @brief Очищает Inspector.
 *
 * Все секции компонентов полностью скрываются.
 */
void InspectorPanel::ClearSelection() {
    selected_object_.reset();

    title_label_->setText(
        "No object selected"
    );

    transform_section_->hide();
    material_section_->hide();
    light_section_->hide();

    material_combo_->clear();

    material_texture_name_->setText(
        "None"
    );

    information_label_->setText(
        "Select an object in Hierarchy to inspect it."
    );
}

/**
 * @brief Устанавливает callback загрузки Texture.
 *
 * @param callback Callback Texture.
 */
void InspectorPanel::SetTextureChangedCallback(std::function<void(Material&, const QString&)> callback) {
    texture_changed_callback_ =
        std::move(callback);
}

/**
 * @brief Подключает PointLight к Inspector.
 *
 * Inspector хранит только невладеющий указатель.
 *
 * @param point_light Источник света SceneViewport.
 */
void InspectorPanel::SetPointLight(PointLight* point_light) {
    point_light_ = point_light;

    if (
        selected_object_ &&
        selected_object_->GetType() ==
        SceneObject::Type::PointLight
    ) {
        UpdateLightFields();
    }
}

/**
 * @brief Устанавливает callback изменения света.
 *
 * @param callback Функция обновления viewport.
 */
void InspectorPanel::SetLightChangedCallback(std::function<void()> callback) {
    light_changed_callback_ =
        std::move(callback);
}

/**
 * @brief Обновляет параметры PointLight.
 *
 * Position здесь отсутствует:
 * он находится в обычном Transform SceneObject.
 */
void InspectorPanel::UpdateLightFields() {
    if (
        !selected_object_ ||
        selected_object_->GetType() != SceneObject::Type::PointLight ||
        point_light_ == nullptr
    ) {
        return;
    }

    updating_fields_ = true;

    light_intensity_->setValue(
        point_light_->GetIntensity()
    );

    light_enabled_->setChecked(
        point_light_->IsEnabled()
    );

    UpdateLightColorButton();

    updating_fields_ = false;
}

/**
 * @brief Обновляет отображаемый цвет PointLight.
 */
void InspectorPanel::UpdateLightColorButton() {
    if (
        point_light_ == nullptr ||
        light_color_button_ == nullptr
    ) {
        return;
    }

    const Vec3& color =
        point_light_->GetColor();

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