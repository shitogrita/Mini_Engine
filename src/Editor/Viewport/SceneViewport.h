#pragma once

#include "Engine/Assets/ImportedMeshData.h"
#include "Engine/Math/Ray.h"
#include "Engine/Math/matrix_types.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneObject.h"

#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QPointF>
#include <QString>
#include <QTimer>

#include <functional>
#include <memory>
#include <optional>


class QLabel;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;


/**
 * @brief Виджет отображения и взаимодействия с 3D-сценой редактора.
 *
 * SceneViewport отвечает за:
 *
 * - отображение объектов Scene;
 * - управление Camera;
 * - переключение Perspective / Orthographic;
 * - отображение Grid и координатных осей;
 * - импорт Mesh в OpenGL;
 * - выбор SceneObject мышью через Ray Picking.
 */
class SceneViewport final
    : public QOpenGLWidget
{
public:

    /**
     * @brief Тип проекции Camera.
     */
    enum class ProjectionMode
    {
        Perspective,
        Orthographic
    };


    /**
     * @brief Callback изменения выбранного объекта.
     *
     * Используется EditorWindow для синхронизации
     * выбора между Viewport, Hierarchy и Inspector.
     */
    using SelectionChangedCallback =
        std::function<
            void(
                std::shared_ptr<SceneObject>
            )
        >;


public:

    /**
     * @brief Создаёт Scene Viewport.
     */
    explicit SceneViewport(
        QWidget* parent = nullptr
    );


    /**
     * @brief Освобождает OpenGL-ресурсы Viewport.
     */
    ~SceneViewport() override;


    /**
     * @brief Загружает и отображает модель.
     *
     * @param file_path Путь к OBJ-файлу.
     */
    void SetDisplayedFile(
        const QString& file_path
    );


    /**
     * @brief Устанавливает тип проекции Camera.
     */
    void SetProjectionMode(
        ProjectionMode mode
    );


    /**
     * @brief Возвращает текущий тип проекции.
     */
    ProjectionMode
    GetProjectionMode() const;


    /**
     * @brief Возвращает Scene.
     */
    Scene&
    GetScene();


    /**
     * @brief Возвращает Scene только для чтения.
     */
    const Scene&
    GetScene() const;


    /**
     * @brief Устанавливает выбранный SceneObject.
     *
     * Используется, например, когда объект
     * выбирается через HierarchyPanel.
     *
     * Этот метод сам не вызывает
     * SelectionChangedCallback.
     */
    void SetSelectedObject(
        std::shared_ptr<SceneObject> object
    );


    /**
     * @brief Возвращает выбранный объект.
     */
    std::shared_ptr<SceneObject>
    GetSelectedObject() const;


    /**
     * @brief Устанавливает callback изменения selection.
     *
     * Callback вызывается, когда объект
     * выбирается непосредственно во Viewport.
     */
    void SetSelectionChangedCallback(
        SelectionChangedCallback callback
    );


private:

    /**
     * @brief Создаёт UI-элементы Viewport.
     */
    void CreateLayout();


    /**
     * @brief Передаёт ожидающий Mesh на GPU
     * после появления OpenGL Context.
     */
    void UploadPendingMesh();


    /**
     * @brief Рассчитывает начальный Transform модели.
     */
    void CalculateModelFit(
        const ImportedMeshData& mesh_data
    );


    /**
     * @brief Обрабатывает непрерывное движение Camera.
     */
    void TickInput();


    /**
     * @brief Обновляет заголовок Scene View.
     */
    void UpdateProjectionTitle();


    /**
     * @brief Обновляет информационный блок координат.
     */
    void UpdateCoordinatesLabel();


    /**
     * @brief Располагает все объекты сцены.
     *
     * Метод сохранён для возможного ручного
     * автоматического выравнивания объектов.
     *
     * При обычном импорте сейчас не используется,
     * чтобы не изменять Transform уже существующих объектов.
     */
    void ArrangeSceneObjects();


    /**
     * @brief Создаёт Grid и мировые оси редактора.
     */
    void CreateEditorGrid();


    /**
     * @brief Создаёт геометрию Grid.
     */
    ImportedMeshData
    CreateGridMeshData() const;


    /**
     * @brief Создаёт одну координатную ось.
     *
     * @param start Начало линии.
     * @param end Конец линии.
     */
    ImportedMeshData
    CreateAxisMeshData(
        const Vec3& start,
        const Vec3& end
    ) const;


    /**
     * @brief Определяет позицию нового импортированного объекта.
     */
    Vec3
    FindSpawnPosition() const;


    /**
     * @brief Создаёт Ray из позиции мыши.
     *
     * Луч строится в World Space.
     *
     * @param mouse_position Координаты мыши внутри Viewport.
     */
    Ray
    CreateMouseRay(
        const QPointF& mouse_position
    ) const;


    /**
     * @brief Выполняет Ray Picking объекта сцены.
     *
     * Для каждого SceneObject:
     *
     * 1. World Ray переводится в Local Space;
     * 2. проверяется пересечение с BoundingBox;
     * 3. выбирается ближайший объект.
     *
     * @param mouse_position Позиция клика мыши.
     */
    void SelectObjectAt(
        const QPointF& mouse_position
    );


    /**
     * @brief Сообщает EditorWindow
     * об изменении выбранного объекта.
     */
    void NotifySelectionChanged();


protected:

    /**
     * @brief Инициализация OpenGL.
     */
    void initializeGL() override;


    /**
     * @brief Обработка изменения размера Viewport.
     */
    void resizeGL(
        int width,
        int height
    ) override;


    /**
     * @brief Отрисовка Scene.
     */
    void paintGL() override;


    /**
     * @brief Обработка нажатия клавиши.
     */
    void keyPressEvent(
        QKeyEvent* event
    ) override;


    /**
     * @brief Обработка отпускания клавиши.
     */
    void keyReleaseEvent(
        QKeyEvent* event
    ) override;


    /**
     * @brief Обработка нажатия кнопки мыши.
     *
     * Обычный ЛКМ — выбор объекта.
     *
     * Alt + ЛКМ — вращение Camera.
     */
    void mousePressEvent(
        QMouseEvent* event
    ) override;


    /**
     * @brief Обработка отпускания кнопки мыши.
     */
    void mouseReleaseEvent(
        QMouseEvent* event
    ) override;


    /**
     * @brief Обработка движения мыши.
     */
    void mouseMoveEvent(
        QMouseEvent* event
    ) override;


    /**
     * @brief Обработка колеса мыши / trackpad.
     */
    void wheelEvent(
        QWheelEvent* event
    ) override;


private:

    /**
     * @brief Заголовок Scene View.
     */
    QLabel* title_label_ =
        nullptr;


    /**
     * @brief Текст-заглушка до загрузки модели.
     */
    QLabel* content_label_ =
        nullptr;


    /**
     * @brief Информационный блок координат.
     */
    QLabel* coordinates_label_ =
        nullptr;


    /**
     * @brief Renderer движка.
     */
    Renderer renderer_;


    /**
     * @brief Camera редактора.
     */
    Camera camera_;


    /**
     * @brief Текущая Scene.
     */
    Scene scene_;


    /**
     * @brief Выбранный объект сцены.
     */
    std::shared_ptr<SceneObject>
        selected_object_;


    /**
     * @brief Callback выбора объекта
     * непосредственно во Viewport.
     */
    SelectionChangedCallback
        selection_changed_callback_;


    /**
     * @brief Основной Shader.
     */
    std::unique_ptr<Shader>
        shader_;


    /**
     * @brief Mesh Grid.
     */
    std::unique_ptr<Mesh>
        grid_mesh_;


    /**
     * @brief Mesh оси X.
     */
    std::unique_ptr<Mesh>
        axis_x_mesh_;


    /**
     * @brief Mesh оси Y.
     */
    std::unique_ptr<Mesh>
        axis_y_mesh_;


    /**
     * @brief Mesh оси Z.
     */
    std::unique_ptr<Mesh>
        axis_z_mesh_;


    /**
     * @brief Mesh, ожидающий загрузки в OpenGL.
     */
    std::optional<ImportedMeshData>
        pending_mesh_data_;


    /**
     * @brief Путь к последнему открытому файлу.
     */
    QString current_file_path_;


    /**
     * @brief Цвет фона Scene.
     */
    Vec3 background_color_{
        0.12f,
        0.13f,
        0.14f
    };


    /**
     * @brief Начальная позиция импортируемой модели.
     */
    Vec3 model_position_{
        0.0f,
        0.0f,
        0.0f
    };


    /**
     * @brief Начальное вращение импортируемой модели.
     */
    Vec3 model_rotation_{
        0.0f,
        0.0f,
        0.0f
    };


    /**
     * @brief Начальный Scale импортируемой модели.
     */
    Vec3 model_scale_{
        1.0f,
        1.0f,
        1.0f
    };


    /**
     * @brief Флаги непрерывного движения Camera.
     */
    bool move_forward_ =
        false;

    bool move_backward_ =
        false;

    bool move_left_ =
        false;

    bool move_right_ =
        false;

    bool move_up_ =
        false;

    bool move_down_ =
        false;


    /**
     * @brief Активно ли вращение Camera мышью.
     */
    bool pointer_look_active_ =
        false;


    /**
     * @brief Предыдущая позиция мыши
     * во время вращения Camera.
     */
    QPointF last_pointer_position_;


    /**
     * @brief Таймер обновления Camera.
     */
    QTimer input_timer_;


    /**
     * @brief Таймер вычисления delta time.
     */
    QElapsedTimer input_clock_;


    /**
     * @brief Текущий режим Projection.
     */
    ProjectionMode projection_mode_ =
        ProjectionMode::Perspective;


    /**
     * @brief Половина вертикального размера
     * Orthographic projection.
     */
    float orthographic_half_height_ =
        5.0f;


    /**
     * @brief Показывает, был ли уже создан OpenGL Context.
     */
    bool gl_initialized_ =
        false;
};