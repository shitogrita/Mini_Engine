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
 * @brief Inspector выбранного объекта сцены.
 *
 * Inspector отображает разные компоненты
 * в зависимости от типа выбранного SceneObject.
 *
 * Для обычного объекта:
 *
 * Transform
 * Material
 *
 * Для Point Light:
 *
 * Transform
 * Light
 *
 * Таким образом настройки источника света
 * не отображаются у обычных объектов сцены.
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
     * Inspector автоматически определяет тип объекта
     * и показывает подходящие компоненты.
     *
     * @param object Выбранный объект или nullptr.
     */
    void SetSelectedObject(std::shared_ptr<SceneObject> object);

    /**
     * @brief Обновляет значения Transform.
     *
     * Используется после изменения объекта
     * через Gizmo во Viewport.
     */
    void RefreshTransformFields();

    /**
     * @brief Устанавливает callback изменения Transform или Material.
     *
     * Callback используется для перерисовки viewport.
     *
     * @param callback Функция, вызываемая после изменения.
     */
    void SetTransformChangedCallback(std::function<void()> callback);

    /**
     * @brief Устанавливает callback выбора Texture.
     *
     * Реальную загрузку Texture выполняет SceneViewport,
     * потому что там находится OpenGL Context.
     *
     * @param callback Callback загрузки Texture.
     */
    void SetTextureChangedCallback(std::function<void(Material&, const QString&)> callback);

    /**
     * @brief Подключает данные PointLight к Inspector.
     *
     * Inspector не владеет PointLight.
     * Источник света принадлежит SceneViewport.
     *
     * @param point_light Указатель на PointLight.
     */
    void SetPointLight(PointLight* point_light);

    /**
     * @brief Устанавливает callback изменения PointLight.
     *
     * @param callback Функция перерисовки viewport.
     */
    void SetLightChangedCallback(std::function<void()> callback);

    /**
     * @brief Очищает selection Inspector.
     */
    void ClearSelection();

private:
    /**
     * @brief Создаёт UI Inspector.
     */
    void CreateLayout();

    /**
     * @brief Создаёт поле числового значения Transform.
     *
     * @return Новый QDoubleSpinBox.
     */
    QDoubleSpinBox* CreateTransformSpinBox();

    /**
     * @brief Создаёт поле числового параметра Material или Light.
     *
     * @param minimum Минимальное значение.
     * @param maximum Максимальное значение.
     * @param step Шаг изменения.
     *
     * @return Новый QDoubleSpinBox.
     */
    QDoubleSpinBox* CreateMaterialSpinBox(double minimum, double maximum, double step);

    /**
     * @brief Обновляет Transform Inspector.
     */
    void UpdateTransformFields();

    /**
     * @brief Обновляет список материалов выбранного объекта.
     *
     * Для multipart OBJ каждая RenderPart
     * получает собственный элемент списка.
     */
    void UpdateMaterialList();

    /**
     * @brief Возвращает выбранный Material.
     *
     * Для обычного объекта возвращается Material SceneObject.
     * Для multipart-модели возвращается Material выбранной RenderPart.
     *
     * @return Material или nullptr.
     */
    Material* GetSelectedMaterial();

    /**
     * @brief Обновляет UI текущего Material.
     */
    void UpdateMaterialFields();

    /**
     * @brief Обновляет UI текущего PointLight.
     */
    void UpdateLightFields();

    /**
     * @brief Обновляет цвет кнопки PointLight.
     */
    void UpdateLightColorButton();

    /**
     * @brief Устанавливает заголовок выбранного объекта.
     *
     * @param object_name Имя SceneObject.
     */
    void SetObjectName(const QString& object_name);

    /**
     * @brief Выбранный SceneObject.
     */
    std::shared_ptr<SceneObject> selected_object_;

    /**
     * @brief Невладеющий указатель на данные текущего PointLight.
     */
    PointLight* point_light_ = nullptr;

    /**
     * @brief Заголовок Inspector.
     */
    QLabel* title_label_ = nullptr;

    /**
	 * @brief Информационный текст Inspector.
	 *
	 * Показывает краткое описание текущего выбранного объекта
	 * или сообщение о том, что объект не выбран.
	 */
    QLabel* information_label_ = nullptr;

    /**
     * @brief Контейнер Transform.
     *
     * Скрывается целиком при отсутствии selection.
     */
    QWidget* transform_section_ = nullptr;

    /**
     * @brief Заголовок Transform.
     */
    QLabel* transform_label_ = nullptr;

    /**
     * @brief Position X.
     */
    QDoubleSpinBox* position_x_ = nullptr;

    /**
     * @brief Position Y.
     */
    QDoubleSpinBox* position_y_ = nullptr;

    /**
     * @brief Position Z.
     */
    QDoubleSpinBox* position_z_ = nullptr;

    /**
     * @brief Rotation X.
     */
    QDoubleSpinBox* rotation_x_ = nullptr;

    /**
     * @brief Rotation Y.
     */
    QDoubleSpinBox* rotation_y_ = nullptr;

    /**
     * @brief Rotation Z.
     */
    QDoubleSpinBox* rotation_z_ = nullptr;

    /**
     * @brief Scale X.
     */
    QDoubleSpinBox* scale_x_ = nullptr;

    /**
     * @brief Scale Y.
     */
    QDoubleSpinBox* scale_y_ = nullptr;

    /**
     * @brief Scale Z.
     */
    QDoubleSpinBox* scale_z_ = nullptr;

    /**
     * @brief Контейнер всего Material Inspector.
     *
     * Для Point Light полностью скрывается.
     */
    QWidget* material_section_ = nullptr;

    /**
     * @brief Заголовок Material.
     */
    QLabel* material_label_ = nullptr;

    /**
     * @brief Выбор RenderPart multipart-модели.
     */
    QComboBox* material_combo_ = nullptr;

    /**
     * @brief Кнопка выбора цвета Material.
     */
    QPushButton* material_color_button_ = nullptr;

    /**
     * @brief Кнопка выбора Texture.
     */
    QPushButton* material_texture_button_ = nullptr;

    /**
     * @brief Имя выбранной Texture.
     */
    QLabel* material_texture_name_ = nullptr;

    /**
     * @brief Ambient strength.
     */
    QDoubleSpinBox* material_ambient_ = nullptr;

    /**
     * @brief Diffuse strength.
     */
    QDoubleSpinBox* material_diffuse_ = nullptr;

    /**
     * @brief Specular strength.
     */
    QDoubleSpinBox* material_specular_ = nullptr;

    /**
     * @brief Shininess.
     */
    QDoubleSpinBox* material_shininess_ = nullptr;

    /**
     * @brief Контейнер параметров Point Light.
     *
     * Показывается только при выборе SceneObject
     * типа PointLight.
     */
    QWidget* light_section_ = nullptr;

    /**
     * @brief Заголовок Light.
     */
    QLabel* light_label_ = nullptr;

    /**
     * @brief Кнопка выбора цвета света.
     */
    QPushButton* light_color_button_ = nullptr;

    /**
     * @brief Интенсивность света.
     */
    QDoubleSpinBox* light_intensity_ = nullptr;

    /**
     * @brief Включён ли источник.
     */
    QCheckBox* light_enabled_ = nullptr;

    /**
     * @brief Защита от обработки valueChanged
     * при программном обновлении UI.
     */
    bool updating_fields_ = false;

    /**
     * @brief Callback изменения объекта.
     */
    std::function<void()> transform_changed_callback_;

    /**
     * @brief Callback загрузки Texture.
     */
    std::function<void(Material&, const QString&)> texture_changed_callback_;

    /**
     * @brief Callback изменения PointLight.
     */
    std::function<void()> light_changed_callback_;
};