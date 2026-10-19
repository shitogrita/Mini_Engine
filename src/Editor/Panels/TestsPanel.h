#pragma once

#include "Engine/Core/ExecutionMode.h"
#include "Engine/Tests/TestScene.h"

#include <QWidget>

#include <cstddef>
#include <functional>

class QComboBox;
class QLabel;
class QPushButton;

/**
 * @brief Компактная встроенная панель запуска базового нагрузочного теста.
 *
 * TestsPanel является отдельным UI-компонентом и не выполняет benchmark сам.
 * Он предоставляет выбор тестовой сцены и режима обновления, кнопки Run/Stop
 * и базовые показатели производительности.
 *
 * В предоставленной реализации TestsPanel поддерживает только Cube1k и не
 * предоставляет отдельный выбор количества workers. Более полный интерфейс
 * тестирования реализован в TestsDialog.
 */
class TestsPanel final : public QWidget {
public:
    /**
     * @brief Создаёт панель тестирования.
     */
    explicit TestsPanel(QWidget* parent = nullptr);

    /**
     * @brief Устанавливает callback запуска теста.
     */
    void SetRunTestCallback(std::function<void(TestScene, ExecutionMode)> callback);

    /**
     * @brief Устанавливает callback остановки теста.
     */
    void SetStopTestCallback(std::function<void()> callback);

    /**
     * @brief Обновляет базовые показатели производительности.
     */
    void SetPerformanceStats(std::size_t object_count, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double fps);

    /**
     * @brief Сбрасывает отображаемые показатели к начальному состоянию.
     */
    void ResetPerformanceStats();

    /**
     * @brief Меняет доступность элементов управления во время теста.
     */
    void SetRunning(bool running);

private:
    /**
     * @brief Возвращает выбранную тестовую сцену.
     */
    TestScene GetSelectedTestScene() const;

    /**
     * @brief Возвращает выбранный режим выполнения.
     */
    ExecutionMode GetSelectedExecutionMode() const;

private:
    QComboBox* test_scene_combo_ = nullptr;
    QComboBox* execution_mode_combo_ = nullptr;

    QPushButton* run_button_ = nullptr;
    QPushButton* stop_button_ = nullptr;

    QLabel* objects_value_ = nullptr;
    QLabel* workers_value_ = nullptr;
    QLabel* update_value_ = nullptr;
    QLabel* render_value_ = nullptr;
    QLabel* frame_value_ = nullptr;
    QLabel* fps_value_ = nullptr;

    std::function<void(TestScene, ExecutionMode)> run_test_callback_;
    std::function<void()> stop_test_callback_;
};
