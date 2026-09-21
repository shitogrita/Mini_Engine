#pragma once

#include "Engine/Scene/SceneObject.h"

#include <QWidget>

#include <functional>
#include <memory>

class QLabel;
class QPushButton;
class QDoubleSpinBox;
class QComboBox;

class InspectorPanel : public QWidget {
public:
    class QLineEdit;
    explicit InspectorPanel(QWidget* parent = nullptr);

    void SetObjectName(const QString& object_name);
    void SetSelectedObject(std::shared_ptr<SceneObject> object);
    void ClearSelection();

    void SetTransformChangedCallback(std::function<void()> callback);
    void RefreshTransformFields();

    void SetTextureChangedCallback(std::function<void(Material&, const QString&)> callback);

private:
    void CreateLayout();

    void UpdateTransformFields();
    void UpdateMaterialList();
    void UpdateMaterialFields();

    Material* GetSelectedMaterial();

    QDoubleSpinBox* CreateTransformSpinBox();

    QDoubleSpinBox* CreateMaterialSpinBox(
        double minimum,
        double maximum,
        double step
    );

private:
    QLabel* title_label_ = nullptr;
    QLabel* transform_label_ = nullptr;
    QLabel* material_label_ = nullptr;
    QLabel* information_label_ = nullptr;

    QDoubleSpinBox* position_x_ = nullptr;
    QDoubleSpinBox* position_y_ = nullptr;
    QDoubleSpinBox* position_z_ = nullptr;

    QDoubleSpinBox* rotation_x_ = nullptr;
    QDoubleSpinBox* rotation_y_ = nullptr;
    QDoubleSpinBox* rotation_z_ = nullptr;

    QDoubleSpinBox* scale_x_ = nullptr;
    QDoubleSpinBox* scale_y_ = nullptr;
    QDoubleSpinBox* scale_z_ = nullptr;

    QComboBox* material_combo_ = nullptr;

    QPushButton* material_color_button_ = nullptr;

    QDoubleSpinBox* material_ambient_ = nullptr;
    QDoubleSpinBox* material_diffuse_ = nullptr;
    QDoubleSpinBox* material_specular_ = nullptr;
    QDoubleSpinBox* material_shininess_ = nullptr;

    QPushButton* material_texture_button_ = nullptr;
    QLabel* material_texture_name_ = nullptr;

    std::function<void(Material&, const QString&)> texture_changed_callback_;


    std::shared_ptr<SceneObject> selected_object_;

    std::function<void()> transform_changed_callback_;

    bool updating_fields_ = false;
};