#include "Editor/Panels/InspectorPanel.h"

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QAbstractSpinBox>
#include <QScrollArea>
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
    setObjectName("InspectorPanel");

    /**
     * Все дочерние элементы получают один базовый шрифт от InspectorPanel.
     * Размеры и начертания отдельных заголовков меняются только через style,
     * поэтому QLabel, QDoubleSpinBox, QComboBox и QPushButton визуально
     * принадлежат одной типографической системе.
     */
    QFont interface_font = font();
    interface_font.setPointSize(13);
    interface_font.setWeight(QFont::Normal);
    setFont(interface_font);

    /**
     * Внутри Inspector используется собственная нейтральная палитра.
     *
     * Ключевой принцип: фон есть у секции и у редактируемого control, но
     * отсутствует у обычных текстовых QLabel. Благодаря этому подписи строк
     * не выглядят как отдельные прямоугольные ячейки внутри rounded-card.
     */
    setStyleSheet(
        R"(
            QWidget#InspectorPanel {
                background-color: #232529;
                color: #d6d8dc;
            }

            QScrollArea#InspectorScroll {
                background-color: #232529;
                border: none;
            }

            QWidget#InspectorContent {
                background-color: #232529;
            }

            QWidget#InspectorPanel QLabel {
                background-color: transparent;
                border: none;
                color: #d6d8dc;
                font-size: 13px;
                font-weight: 400;
            }

            QLabel#InspectorTitle {
                color: #f0f1f2;
                font-size: 17px;
                font-weight: 600;
                padding: 2px 0px 5px 8px;
            }

            QFrame#InspectorSeparator {
                color: #3a3d42;
                background-color: #3a3d42;
                max-height: 1px;
                min-height: 1px;
                border: none;
            }

            QWidget#InspectorSection {
                background-color: #292b2f;
                border: 1px solid #3b3f45;
                border-radius: 9px;
            }

            QLabel#SectionTitle {
                background-color: transparent;
                border: none;
                color: #aeb3ba;
                font-size: 12px;
                font-weight: 600;
                letter-spacing: 0.8px;
                padding: 1px 0px 4px 8px;
            }

            QLabel#FieldLabel {
                background-color: transparent;
                border: none;
                color: #d7dade;
                font-size: 13px;
                font-weight: 400;
                padding-left: 8px;
            }

            QLabel#AxisHeader {
                background-color: transparent;
                border: none;
                color: #9499a1;
                font-size: 12px;
                font-weight: 600;
            }

            QLabel#SecondaryText {
                background-color: transparent;
                border: none;
                color: #92979e;
                font-size: 13px;
                font-weight: 400;
                padding-left: 8px;
            }

            QLabel#InspectorInfo {
                background-color: transparent;
                border: none;
                color: #8f949b;
                font-size: 13px;
                font-weight: 400;
                line-height: 1.3;
                padding: 8px;
            }

            QDoubleSpinBox {
                background-color: #1f2124;
                color: #e0e2e5;
                border: 1px solid #454a51;
                border-radius: 6px;
                padding: 5px 8px;
                min-height: 26px;
                font-size: 13px;
                font-weight: 400;
            }

            QDoubleSpinBox QLineEdit {
                background-color: transparent;
                color: #e0e2e5;
                border: none;
                padding: 0px;
                margin: 0px;
                font-size: 13px;
                font-weight: 400;
                selection-background-color: transparent;
                selection-color: #e0e2e5;
            }

            QDoubleSpinBox:hover {
                border-color: #5b6068;
            }

            QDoubleSpinBox:focus {
                border-color: #747a83;
                background-color: #1f2124;
            }

            QDoubleSpinBox::up-button,
            QDoubleSpinBox::down-button {
                width: 0px;
                height: 0px;
                border: none;
            }

            QComboBox {
                background-color: #1f2124;
                color: #e0e2e5;
                border: 1px solid #454a51;
                border-radius: 6px;
                padding: 5px 8px;
                min-height: 26px;
                font-size: 13px;
                font-weight: 400;
            }

            QComboBox:hover {
                border-color: #5b6068;
            }

            QComboBox:focus {
                border-color: #747a83;
            }

            QComboBox::drop-down {
                border: none;
                width: 22px;
            }

            QComboBox QAbstractItemView {
                background-color: #292b2f;
                color: #d9dbde;
                border: 1px solid #44484e;
                selection-background-color: #4a4e54;
                selection-color: #ffffff;
                outline: none;
                font-size: 13px;
            }

            QPushButton {
                background-color: #34373c;
                color: #e0e2e5;
                border: 1px solid #4a4e55;
                border-radius: 6px;
                padding: 5px 8px;
                min-height: 26px;
                font-size: 13px;
                font-weight: 400;
                text-align: left;
            }

            QPushButton:hover {
                background-color: #3b3f44;
                border-color: #5c6169;
            }

            QPushButton:pressed {
                background-color: #2e3135;
            }

            QCheckBox {
                background-color: transparent;
                color: #d7dade;
                spacing: 7px;
                font-size: 13px;
                font-weight: 400;
                padding-left: 8px;
            }

            QScrollBar:vertical {
                background-color: #232529;
                width: 10px;
                margin: 0px;
            }

            QScrollBar::handle:vertical {
                background-color: #4a4e54;
                border-radius: 5px;
                min-height: 28px;
            }

            QScrollBar::handle:vertical:hover {
                background-color: #5a5f66;
            }

            QScrollBar::add-line:vertical,
            QScrollBar::sub-line:vertical {
                height: 0px;
            }
        )"
    );

    QVBoxLayout* outer_layout = new QVBoxLayout(this);
    outer_layout->setContentsMargins(0, 0, 0, 0);
    outer_layout->setSpacing(0);

    QScrollArea* scroll_area = new QScrollArea(this);
    scroll_area->setObjectName("InspectorScroll");
    scroll_area->setWidgetResizable(true);
    scroll_area->setFrameShape(QFrame::NoFrame);
    scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* content = new QWidget(scroll_area);
    content->setObjectName("InspectorContent");

    QVBoxLayout* main_layout = new QVBoxLayout(content);
    main_layout->setContentsMargins(12, 12, 12, 16);
    main_layout->setSpacing(14);

    title_label_ = new QLabel(content);
    title_label_->setObjectName("InspectorTitle");

    QFrame* separator = new QFrame(content);
    separator->setObjectName("InspectorSeparator");
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain);

    /**
     * Transform присутствует у любого SceneObject, включая Point Light.
     */
    transform_section_ = new QWidget(content);
    transform_section_->setObjectName("InspectorSection");

    QVBoxLayout* transform_section_layout = new QVBoxLayout(transform_section_);
    transform_section_layout->setContentsMargins(12, 10, 12, 12);
    transform_section_layout->setSpacing(9);

    transform_label_ = new QLabel("TRANSFORM", transform_section_);
    transform_label_->setObjectName("SectionTitle");

    QGridLayout* transform_layout = new QGridLayout();
    transform_layout->setHorizontalSpacing(7);
    transform_layout->setVerticalSpacing(8);
    transform_layout->setColumnMinimumWidth(0, 82);
    transform_layout->setColumnStretch(1, 1);
    transform_layout->setColumnStretch(2, 1);
    transform_layout->setColumnStretch(3, 1);

    QLabel* x_label = new QLabel("X", transform_section_);
    QLabel* y_label = new QLabel("Y", transform_section_);
    QLabel* z_label = new QLabel("Z", transform_section_);

    x_label->setObjectName("AxisHeader");
    y_label->setObjectName("AxisHeader");
    z_label->setObjectName("AxisHeader");

    x_label->setAlignment(Qt::AlignCenter);
    y_label->setAlignment(Qt::AlignCenter);
    z_label->setAlignment(Qt::AlignCenter);

    transform_layout->addWidget(CreateFieldLabel("", transform_section_), 0, 0);
    transform_layout->addWidget(x_label, 0, 1);
    transform_layout->addWidget(y_label, 0, 2);
    transform_layout->addWidget(z_label, 0, 3);

    position_x_ = CreateTransformSpinBox();
    position_y_ = CreateTransformSpinBox();
    position_z_ = CreateTransformSpinBox();

    rotation_x_ = CreateTransformSpinBox();
    rotation_y_ = CreateTransformSpinBox();
    rotation_z_ = CreateTransformSpinBox();

    scale_x_ = CreateTransformSpinBox();
    scale_y_ = CreateTransformSpinBox();
    scale_z_ = CreateTransformSpinBox();

    transform_layout->addWidget(CreateFieldLabel("Position", transform_section_), 1, 0);
    transform_layout->addWidget(position_x_, 1, 1);
    transform_layout->addWidget(position_y_, 1, 2);
    transform_layout->addWidget(position_z_, 1, 3);

    transform_layout->addWidget(CreateFieldLabel("Rotation", transform_section_), 2, 0);
    transform_layout->addWidget(rotation_x_, 2, 1);
    transform_layout->addWidget(rotation_y_, 2, 2);
    transform_layout->addWidget(rotation_z_, 2, 3);

    transform_layout->addWidget(CreateFieldLabel("Scale", transform_section_), 3, 0);
    transform_layout->addWidget(scale_x_, 3, 1);
    transform_layout->addWidget(scale_y_, 3, 2);
    transform_layout->addWidget(scale_z_, 3, 3);

    transform_section_layout->addWidget(transform_label_);
    transform_section_layout->addLayout(transform_layout);

    /**
     * Material отображается только для геометрических объектов.
     */
    material_section_ = new QWidget(content);
    material_section_->setObjectName("InspectorSection");

    QVBoxLayout* material_section_layout = new QVBoxLayout(material_section_);
    material_section_layout->setContentsMargins(12, 10, 12, 12);
    material_section_layout->setSpacing(9);

    material_label_ = new QLabel("MATERIAL", material_section_);
    material_label_->setObjectName("SectionTitle");

    QGridLayout* material_layout = new QGridLayout();
    material_layout->setHorizontalSpacing(10);
    material_layout->setVerticalSpacing(8);
    material_layout->setColumnMinimumWidth(0, 90);
    material_layout->setColumnStretch(1, 1);

    material_combo_ = new QComboBox(material_section_);
    material_color_button_ = new QPushButton("Choose...", material_section_);
    material_texture_button_ = new QPushButton("Choose Texture...", material_section_);

    material_texture_name_ = new QLabel("None", material_section_);
    material_texture_name_->setObjectName("SecondaryText");
    material_texture_name_->setWordWrap(true);

    material_ambient_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_diffuse_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_specular_ = CreateMaterialSpinBox(0.0, 1.0, 0.05);
    material_shininess_ = CreateMaterialSpinBox(1.0, 256.0, 1.0);

    material_layout->addWidget(CreateFieldLabel("Part", material_section_), 0, 0);
    material_layout->addWidget(material_combo_, 0, 1);

    material_layout->addWidget(CreateFieldLabel("Color", material_section_), 1, 0);
    material_layout->addWidget(material_color_button_, 1, 1);

    material_layout->addWidget(CreateFieldLabel("Texture", material_section_), 2, 0);
    material_layout->addWidget(material_texture_button_, 2, 1);

    material_layout->addWidget(CreateFieldLabel("", material_section_), 3, 0);
    material_layout->addWidget(material_texture_name_, 3, 1);

    material_layout->addWidget(CreateFieldLabel("Ambient", material_section_), 4, 0);
    material_layout->addWidget(material_ambient_, 4, 1);

    material_layout->addWidget(CreateFieldLabel("Diffuse", material_section_), 5, 0);
    material_layout->addWidget(material_diffuse_, 5, 1);

    material_layout->addWidget(CreateFieldLabel("Specular", material_section_), 6, 0);
    material_layout->addWidget(material_specular_, 6, 1);

    material_layout->addWidget(CreateFieldLabel("Shininess", material_section_), 7, 0);
    material_layout->addWidget(material_shininess_, 7, 1);

    material_section_layout->addWidget(material_label_);
    material_section_layout->addLayout(material_layout);

    /**
     * Point Light использует общий Transform и собственную секцию света.
     */
    light_section_ = new QWidget(content);
    light_section_->setObjectName("InspectorSection");

    QVBoxLayout* light_section_layout = new QVBoxLayout(light_section_);
    light_section_layout->setContentsMargins(12, 10, 12, 12);
    light_section_layout->setSpacing(9);

    light_label_ = new QLabel("POINT LIGHT", light_section_);
    light_label_->setObjectName("SectionTitle");

    light_color_button_ = new QPushButton("Choose...", light_section_);
    light_intensity_ = CreateMaterialSpinBox(0.0, 100.0, 0.1);
    light_enabled_ = new QCheckBox("Enabled", light_section_);

    QGridLayout* light_layout = new QGridLayout();
    light_layout->setHorizontalSpacing(10);
    light_layout->setVerticalSpacing(8);
    light_layout->setColumnMinimumWidth(0, 90);
    light_layout->setColumnStretch(1, 1);

    light_layout->addWidget(CreateFieldLabel("Color", light_section_), 0, 0);
    light_layout->addWidget(light_color_button_, 0, 1);

    light_layout->addWidget(CreateFieldLabel("Intensity", light_section_), 1, 0);
    light_layout->addWidget(light_intensity_, 1, 1);

    light_layout->addWidget(CreateFieldLabel("State", light_section_), 2, 0);
    light_layout->addWidget(light_enabled_, 2, 1);

    light_section_layout->addWidget(light_label_);
    light_section_layout->addLayout(light_layout);

    information_label_ = new QLabel(content);
    information_label_->setObjectName("InspectorInfo");
    information_label_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    information_label_->setWordWrap(true);

    main_layout->addWidget(title_label_);
    main_layout->addWidget(separator);
    main_layout->addWidget(transform_section_);
    main_layout->addWidget(material_section_);
    main_layout->addWidget(light_section_);
    main_layout->addWidget(information_label_);
    main_layout->addStretch();

    scroll_area->setWidget(content);
    outer_layout->addWidget(scroll_area);

    /**
     * Один callback обслуживает все девять полей Transform. После любого
     * изменения полное состояние UI переносится в Transform выбранного
     * SceneObject.
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

    /**
     * При выборе другой RenderPart multipart-модели Inspector начинает
     * отображать Material именно этой части.
     */
    connect(material_combo_, &QComboBox::currentIndexChanged, this, [this](int) {
        if (updating_fields_) {
            return;
        }

        UpdateMaterialFields();
    });

    /**
     * Один callback обслуживает числовые свойства Material.
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

    /**
     * Изменяет базовый цвет текущего Material.
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

    /**
     * Выбор Texture передаётся наружу. Texture2D создаётся в SceneViewport,
     * где доступен активный OpenGL Context.
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

    /**
     * Изменяет интенсивность PointLight выбранного SceneObject.
     */
    connect(light_intensity_, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        if (updating_fields_ || !selected_object_) {
            return;
        }

        PointLight* light = selected_object_->GetPointLight();

        if (!light) {
            return;
        }

        light->SetIntensity(
            static_cast<float>(value)
        );

        if (light_changed_callback_) {
            light_changed_callback_();
        }
    });

    /**
     * Включает или выключает PointLight выбранного SceneObject.
     */
    connect(light_enabled_, &QCheckBox::toggled, this, [this](bool enabled) {
        if (updating_fields_ || !selected_object_) {
            return;
        }

        PointLight* light = selected_object_->GetPointLight();

        if (!light) {
            return;
        }

        light->SetEnabled(
            enabled
        );

        if (light_changed_callback_) {
            light_changed_callback_();
        }
    });

    /**
     * Изменяет цвет PointLight выбранного SceneObject.
     */
    connect(light_color_button_, &QPushButton::clicked, this, [this]() {
        if (!selected_object_) {
            return;
        }

        PointLight* light = selected_object_->GetPointLight();

        if (!light) {
            return;
        }

        const Vec3& color = light->GetColor();

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

        light->SetColor(
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

    /**
     * До первого selection компонентные секции Inspector не отображаются.
     */
    transform_section_->hide();
    material_section_->hide();
    light_section_->hide();
}

/**
 * @brief Создаёт прозрачную подпись строки Inspector.
 *
 * Внутренний отступ слева составляет 8 px, что визуально соответствует
 * примерно одному пробелу перед текстом. Отступ реализован через layout/style,
 * а не добавлением символа пробела в строку, поэтому выравнивание остаётся
 * одинаковым для всех подписей.
 *
 * @param text Текст подписи.
 * @param parent Родительский QWidget.
 * @return Настроенный QLabel.
 */
QLabel* InspectorPanel::CreateFieldLabel(const QString& text, QWidget* parent) const {
    QLabel* label = new QLabel(text, parent);
    label->setObjectName("FieldLabel");
    label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    label->setMinimumHeight(28);
    return label;
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
    spin_box->setMinimumWidth(62);
    spin_box->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spin_box->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    /**
     * QAbstractSpinBox::lineEdit() является protected в Qt 6, поэтому прямой
     * вызов из InspectorPanel невозможен. Внутренний редактор при этом является
     * дочерним QLineEdit, поэтому получаем его через QObject::findChild().
     *
     * Стиль задаётся непосредственно QLineEdit, а цвет выделения совпадает с
     * фоном spin box. Благодаря этому Qt не рисует маленькую прямоугольную
     * плашку вокруг выделенного числа внутри внешнего rounded control.
     */
    if (QLineEdit* editor = spin_box->findChild<QLineEdit*>()) {
        editor->setFrame(false);
        editor->setStyleSheet(
            R"(
                QLineEdit {
                    background-color: transparent;
                    border: none;
                    color: #e0e2e5;
                    padding: 0px;
                    margin: 0px;
                    font-size: 13px;
                    font-weight: 400;
                    selection-background-color: #1f2124;
                    selection-color: #e0e2e5;
                }
            )"
        );

        QPalette editor_palette = editor->palette();
        editor_palette.setColor(QPalette::Base, Qt::transparent);
        editor_palette.setColor(QPalette::Text, QColor("#e0e2e5"));
        editor_palette.setColor(QPalette::Highlight, QColor("#1f2124"));
        editor_palette.setColor(QPalette::HighlightedText, QColor("#e0e2e5"));
        editor->setPalette(editor_palette);
    }

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
    spin_box->setMinimumWidth(92);
    spin_box->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spin_box->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    /**
     * Для Material и Point Light применяется та же настройка внутреннего
     * QLineEdit, что и для Transform. Это гарантирует одинаковую типографику
     * и отсутствие отдельной прямоугольной подсветки текста внутри поля.
     */
    if (QLineEdit* editor = spin_box->findChild<QLineEdit*>()) {
        editor->setFrame(false);
        editor->setStyleSheet(
            R"(
                QLineEdit {
                    background-color: transparent;
                    border: none;
                    color: #e0e2e5;
                    padding: 0px;
                    margin: 0px;
                    font-size: 13px;
                    font-weight: 400;
                    selection-background-color: #1f2124;
                    selection-color: #e0e2e5;
                }
            )"
        );

        QPalette editor_palette = editor->palette();
        editor_palette.setColor(QPalette::Base, Qt::transparent);
        editor_palette.setColor(QPalette::Text, QColor("#e0e2e5"));
        editor_palette.setColor(QPalette::Highlight, QColor("#1f2124"));
        editor_palette.setColor(QPalette::HighlightedText, QColor("#e0e2e5"));
        editor->setPalette(editor_palette);
    }

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

    information_label_->hide();

    const bool is_point_light = selected_object_->HasPointLight();
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

    const QString material_text_color =
        q_color.lightnessF() > 0.62
            ? "#1f2124"
            : "#f2f3f4";

    material_color_button_->setStyleSheet(
        QString(
            "QPushButton {"
            "background-color: %1;"
            "color: %2;"
            "border: 1px solid #5a5f66;"
            "border-radius: 6px;"
            "padding: 5px 8px;"
            "min-height: 26px;"
            "font-size: 13px;"
            "font-weight: 400;"
            "text-align: left;"
            "}"
            "QPushButton:hover {"
            "border-color: #777c84;"
            "}"
        ).arg(q_color.name(), material_text_color)
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

    title_label_->setText("Inspector");

    transform_section_->hide();
    material_section_->hide();
    light_section_->hide();

    material_combo_->clear();

    material_texture_name_->setText(
        "None"
    );

    information_label_->setAlignment(Qt::AlignCenter);
    information_label_->setMinimumHeight(170);
    information_label_->setText(
        "No object selected\n\n"
        "Select an object in Hierarchy or Scene View."
    );
    information_label_->show();
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
 * @brief Устанавливает callback изменения света.
 *
 * @param callback Функция обновления viewport.
 */
void InspectorPanel::SetLightChangedCallback(std::function<void()> callback) {
    light_changed_callback_ =
        std::move(callback);
}

/**
 * @brief Обновляет параметры PointLight выбранного SceneObject.
 */
void InspectorPanel::UpdateLightFields() {
    if (!selected_object_) {
        return;
    }

    PointLight* light =
        selected_object_->GetPointLight();

    if (!light) {
        return;
    }

    updating_fields_ = true;

    light_intensity_->setValue(
        light->GetIntensity()
    );

    light_enabled_->setChecked(
        light->IsEnabled()
    );

    UpdateLightColorButton();

    updating_fields_ = false;
}

/**
 * @brief Обновляет цвет кнопки PointLight.
 */
void InspectorPanel::UpdateLightColorButton() {
    if (!selected_object_ || !light_color_button_) {
        return;
    }

    const PointLight* light =
        selected_object_->GetPointLight();

    if (!light) {
        return;
    }

    const Vec3& color =
        light->GetColor();

    const QColor q_color = QColor::fromRgbF(
        std::clamp(color.x, 0.0f, 1.0f),
        std::clamp(color.y, 0.0f, 1.0f),
        std::clamp(color.z, 0.0f, 1.0f)
    );

    const QString light_text_color =
        q_color.lightnessF() > 0.62
            ? "#1f2124"
            : "#f2f3f4";

    light_color_button_->setStyleSheet(
        QString(
            "QPushButton {"
            "background-color: %1;"
            "color: %2;"
            "border: 1px solid #5a5f66;"
            "border-radius: 6px;"
            "padding: 5px 8px;"
            "min-height: 26px;"
            "font-size: 13px;"
            "font-weight: 400;"
            "text-align: left;"
            "}"
            "QPushButton:hover {"
            "border-color: #777c84;"
            "}"
        ).arg(q_color.name(), light_text_color)
    );
}
