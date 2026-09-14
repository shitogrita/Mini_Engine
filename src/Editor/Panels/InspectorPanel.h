#pragma once

#include "Engine/Scene/SceneObject.h"

#include <QWidget>

#include <functional>
#include <memory>


class QLabel;
class QPushButton;
class QDoubleSpinBox;


/**
 * @brief Панель свойств выбранного объекта сцены.
 *
 * InspectorPanel отображает Transform и Material
 * выбранного SceneObject и позволяет изменять их значения.
 */
class InspectorPanel : public QWidget {
public:
    explicit InspectorPanel(QWidget* parent = nullptr);

    /**
     * @brief Устанавливает отображаемое имя объекта.
     */
    void SetObjectName(const QString& object_name);

    /**
     * @brief Устанавливает объект,
     * свойства которого отображаются в Inspector.
     */
    void SetSelectedObject(std::shared_ptr<SceneObject> object);

    /**
     * @brief Сбрасывает текущий selection.
     */
    void ClearSelection();

    /**
     * @brief Устанавливает callback,
     * вызываемый после изменения объекта
     * пользователем через Inspector.
     */
    void SetTransformChangedCallback(std::function<void()> callback);

    /**
     * @brief Повторно считывает Transform
     * выбранного SceneObject и обновляет поля.
     */
    void RefreshTransformFields();


private:
    void CreateLayout();

    void UpdateTransformFields();
    void UpdateMaterialFields();

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


    QPushButton* material_color_button_ = nullptr;

    QDoubleSpinBox* material_ambient_ = nullptr;
    QDoubleSpinBox* material_diffuse_ = nullptr;
    QDoubleSpinBox* material_specular_ = nullptr;
    QDoubleSpinBox* material_shininess_ = nullptr;


    std::shared_ptr<SceneObject> selected_object_;

    std::function<void()> transform_changed_callback_;


    /**
     * @brief Защищает от обратного изменения
     * SceneObject во время программного обновления UI.
     */
    bool updating_fields_ = false;
};