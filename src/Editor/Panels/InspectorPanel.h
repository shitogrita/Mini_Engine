#pragma once

#include "Engine/Renderer/Material.h"
#include "Engine/Scene/PointLight.h"
#include "Engine/Scene/SceneObject.h"

#include <QWidget>

#include <functional>
#include <memory>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QString;

/**
 * @brief Inspector выбранного объекта и параметров сцены.
 *
 * Панель позволяет редактировать:
 *
 * SceneObject:
 * - Transform;
 * - Material;
 * - Texture.
 *
 * PointLight:
 * - Position;
 * - Color;
 * - Intensity;
 * - Enabled.
 */
class InspectorPanel final : public QWidget {
public:
    /**
     * @brief Создаёт Inspector.
     *
     * @param parent Родительский QWidget.
     */
    explicit InspectorPanel(QWidget* parent = nullptr);

    /**
     * @brief Устанавливает выбранный SceneObject.
     *
     * @param object Объект сцены или nullptr.
     */
    void SetSelectedObject(std::shared_ptr<SceneObject> object);

    /**
     * @brief Обновляет поля Transform после изменения объекта
     * вне Inspector, например через Gizmo.
     */
    void RefreshTransformFields();

    /**
     * @brief Устанавливает callback изменения SceneObject.
     *
     * @param callback Функция, вызываемая после изменения данных.
     */
    void SetTransformChangedCallback(std::function<void()> callback);

    /**
     * @brief Устанавливает callback выбора новой Texture.
     *
     * @param callback Функция загрузки Texture через SceneViewport.
     */
    void SetTextureChangedCallback(std::function<void(Material&, const QString&)> callback);

    /**
     * @brief Подключает PointLight к Inspector.
     *
     * PointLight принадлежит SceneViewport.
     * Inspector хранит только невладеющий указатель.
     *
     * @param point_light Источник света.
     */
    void SetPointLight(PointLight* point_light);

    /**
     * @brief Устанавливает callback изменения PointLight.
     *
     * Используется для немедленной перерисовки viewport.
     *
     * @param callback Callback изменения света.
     */
    void SetLightChangedCallback(std::function<void()> callback);

    /**
     * @brief Сбрасывает выбранный SceneObject.
     */
    void ClearSelection();

private:
    /**
     * @brief Создаёт элементы Inspector.
     */
    void CreateLayout();

    /**
     * @brief Создаёт SpinBox для Transform.
     */
    QDoubleSpinBox* CreateTransformSpinBox();

    /**
     * @brief Создаёт SpinBox для Material.
     */
    QDoubleSpinBox* CreateMaterialSpinBox(double minimum, double maximum, double step);

    /**
     * @brief Обновляет поля Transform.
     */
    void UpdateTransformFields();

    /**
     * @brief Обновляет список multipart-материалов.
     */
    void UpdateMaterialList();

    /**
     * @brief Возвращает выбранный Material.
     *
     * Для multipart-модели Material определяется
     * текущим значением material_combo_.
     *
     * @return Material или nullptr.
     */
    Material* GetSelectedMaterial();

    /**
     * @brief Обновляет поля Material.
     */
    void UpdateMaterialFields();

    /**
     * @brief Обновляет поля PointLight.
     */
    void UpdateLightFields();

    /**
     * @brief Обновляет визуальный цвет кнопки PointLight.
     */
    void UpdateLightColorButton();

    /**
     * @brief Изменяет заголовок выбранного SceneObject.
     */
    void SetObjectName(const QString& object_name);

    std::shared_ptr<SceneObject> selected_object_;

    PointLight* point_light_ = nullptr;

    QLabel* title_label_ = nullptr;
    QLabel* transform_label_ = nullptr;
    QLabel* material_label_ = nullptr;
    QLabel* light_label_ = nullptr;
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
    QPushButton* material_texture_button_ = nullptr;
    QLabel* material_texture_name_ = nullptr;

    QDoubleSpinBox* material_ambient_ = nullptr;
    QDoubleSpinBox* material_diffuse_ = nullptr;
    QDoubleSpinBox* material_specular_ = nullptr;
    QDoubleSpinBox* material_shininess_ = nullptr;

    QDoubleSpinBox* light_position_x_ = nullptr;
    QDoubleSpinBox* light_position_y_ = nullptr;
    QDoubleSpinBox* light_position_z_ = nullptr;
    QDoubleSpinBox* light_intensity_ = nullptr;

    QPushButton* light_color_button_ = nullptr;
    QCheckBox* light_enabled_ = nullptr;

    bool updating_fields_ = false;

    std::function<void()> transform_changed_callback_;
    std::function<void(Material&, const QString&)> texture_changed_callback_;
    std::function<void()> light_changed_callback_;
};