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
 * @brief Панель редактирования компонентов выбранного SceneObject.
 *
 * InspectorPanel отображает свойства выбранного объекта сцены и напрямую
 * изменяет его прикладные данные. Набор секций зависит от типа объекта:
 * - Transform отображается для любого SceneObject;
 * - Material отображается для объектов с геометрией;
 * - Point Light отображается для SceneObject с компонентом PointLight.
 *
 * Для импортированной multipart-модели Inspector позволяет переключаться
 * между RenderPart и редактировать Material каждой части отдельно.
 *
 * Визуально Inspector построен как набор нейтральных серых карточек. Фон
 * принадлежит только самой карточке и редактируемым полям. Текстовые подписи
 * остаются прозрачными, поэтому внутри закруглённой секции не появляются
 * дополнительные прямоугольные плашки.
 *
 * Inspector не выполняет OpenGL-операции самостоятельно. Загрузка Texture
 * передаётся в SceneViewport через callback, потому что именно SceneViewport
 * управляет активным OpenGL Context.
 */
class InspectorPanel final : public QWidget {
public:
    /**
     * @brief Создаёт Inspector.
     *
     * При создании панель находится в пустом состоянии до первого выбора
     * SceneObject.
     *
     * @param parent Родительский QWidget.
     */
    explicit InspectorPanel(QWidget* parent = nullptr);

    /**
     * @brief Устанавливает объект, отображаемый в Inspector.
     *
     * При nullptr панель переходит в пустое состояние и скрывает секции
     * Transform, Material и Point Light.
     *
     * @param object Новый выбранный объект или nullptr.
     */
    void SetSelectedObject(std::shared_ptr<SceneObject> object);

    /**
     * @brief Обновляет поля Transform из текущего состояния SceneObject.
     *
     * Используется после изменения Transform непосредственно во Viewport,
     * например через Move, Rotate или Scale Gizmo.
     */
    void RefreshTransformFields();

    /**
     * @brief Устанавливает callback изменения Transform или Material.
     *
     * Callback используется EditorWindow для запроса перерисовки SceneViewport.
     *
     * @param callback Функция без аргументов.
     */
    void SetTransformChangedCallback(std::function<void()> callback);

    /**
     * @brief Устанавливает callback выбора Texture для Material.
     *
     * Реальная загрузка Texture выполняется SceneViewport при активном OpenGL
     * Context.
     *
     * @param callback Функция, принимающая Material и выбранный путь к файлу.
     */
    void SetTextureChangedCallback(std::function<void(Material&, const QString&)> callback);

    /**
     * @brief Устанавливает callback изменения параметров Point Light.
     *
     * @param callback Функция обновления SceneViewport.
     */
    void SetLightChangedCallback(std::function<void()> callback);

    /**
     * @brief Сбрасывает selection и переводит Inspector в пустое состояние.
     */
    void ClearSelection();

private:
    /**
     * @brief Создаёт полный интерфейс Inspector и подключает UI callbacks.
     */
    void CreateLayout();

    /**
     * @brief Создаёт обычную текстовую подпись строки Inspector.
     *
     * Label имеет прозрачный фон и небольшой левый внутренний отступ. Это
     * сохраняет единый ритм текста, не создавая дополнительных прямоугольных
     * элементов внутри закруглённой секции.
     *
     * @param text Текст подписи.
     * @param parent Родительский QWidget.
     * @return Настроенный QLabel.
     */
    QLabel* CreateFieldLabel(const QString& text, QWidget* parent) const;

    /**
     * @brief Создаёт числовое поле Transform.
     *
     * @return Настроенный QDoubleSpinBox.
     */
    QDoubleSpinBox* CreateTransformSpinBox();

    /**
     * @brief Создаёт числовое поле Material или Point Light.
     *
     * @param minimum Минимальное допустимое значение.
     * @param maximum Максимальное допустимое значение.
     * @param step Шаг изменения.
     * @return Настроенный QDoubleSpinBox.
     */
    QDoubleSpinBox* CreateMaterialSpinBox(double minimum, double maximum, double step);

    /**
     * @brief Копирует Transform выбранного SceneObject в поля Inspector.
     */
    void UpdateTransformFields();

    /**
     * @brief Заполняет список Material выбранного объекта.
     *
     * Для обычного объекта создаётся один пункт. Для импортированной
     * multipart-модели создаётся отдельный пункт на каждый RenderPart.
     */
    void UpdateMaterialList();

    /**
     * @brief Возвращает Material, который сейчас редактируется в Inspector.
     *
     * @return Указатель на Material или nullptr.
     */
    Material* GetSelectedMaterial();

    /**
     * @brief Копирует параметры текущего Material в UI.
     */
    void UpdateMaterialFields();

    /**
     * @brief Копирует параметры Point Light в UI.
     */
    void UpdateLightFields();

    /**
     * @brief Обновляет цвет кнопки выбора цвета Point Light.
     */
    void UpdateLightColorButton();

    /**
     * @brief Устанавливает имя текущего SceneObject в заголовок Inspector.
     *
     * @param object_name Имя объекта.
     */
    void SetObjectName(const QString& object_name);

private:
    /**
     * @brief SceneObject, параметры которого сейчас отображаются.
     */
    std::shared_ptr<SceneObject> selected_object_;

    /**
     * @brief Имя выбранного объекта или заголовок пустого состояния.
     */
    QLabel* title_label_ = nullptr;

    /**
     * @brief Текст пустого состояния Inspector.
     */
    QLabel* information_label_ = nullptr;

    /**
     * @brief Секция Position, Rotation и Scale.
     */
    QWidget* transform_section_ = nullptr;
    QLabel* transform_label_ = nullptr;

    QDoubleSpinBox* position_x_ = nullptr;
    QDoubleSpinBox* position_y_ = nullptr;
    QDoubleSpinBox* position_z_ = nullptr;

    QDoubleSpinBox* rotation_x_ = nullptr;
    QDoubleSpinBox* rotation_y_ = nullptr;
    QDoubleSpinBox* rotation_z_ = nullptr;

    QDoubleSpinBox* scale_x_ = nullptr;
    QDoubleSpinBox* scale_y_ = nullptr;
    QDoubleSpinBox* scale_z_ = nullptr;

    /**
     * @brief Секция Material геометрического объекта.
     */
    QWidget* material_section_ = nullptr;
    QLabel* material_label_ = nullptr;
    QComboBox* material_combo_ = nullptr;
    QPushButton* material_color_button_ = nullptr;
    QPushButton* material_texture_button_ = nullptr;
    QLabel* material_texture_name_ = nullptr;
    QDoubleSpinBox* material_ambient_ = nullptr;
    QDoubleSpinBox* material_diffuse_ = nullptr;
    QDoubleSpinBox* material_specular_ = nullptr;
    QDoubleSpinBox* material_shininess_ = nullptr;

    /**
     * @brief Секция параметров Point Light.
     */
    QWidget* light_section_ = nullptr;
    QLabel* light_label_ = nullptr;
    QPushButton* light_color_button_ = nullptr;
    QDoubleSpinBox* light_intensity_ = nullptr;
    QCheckBox* light_enabled_ = nullptr;

    /**
     * @brief Защищает callbacks от valueChanged во время программного
     * обновления полей Inspector.
     */
    bool updating_fields_ = false;

    /**
     * @brief Callback изменения Transform или Material.
     */
    std::function<void()> transform_changed_callback_;

    /**
     * @brief Callback выбора Texture.
     */
    std::function<void(Material&, const QString&)> texture_changed_callback_;

    /**
     * @brief Callback изменения Point Light.
     */
    std::function<void()> light_changed_callback_;
};
