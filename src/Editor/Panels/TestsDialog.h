#pragma once

#include "Engine/Core/ExecutionMode.h"
#include "Engine/Tests/TestScene.h"

#include <QDialog>

#include <cstddef>
#include <functional>

class QComboBox;
class QLabel;
class QMouseEvent;
class QPushButton;

/**
 * @brief Отдельное окно управления нагрузочными тестами Mini Engine.
 *
 * TestsDialog предоставляет пользовательский интерфейс для запуска benchmark,
 * но не выполняет измерения самостоятельно. Реальная тестовая Scene создаётся
 * SceneViewport, там же выполняются SingleThreaded и MultiThreaded обновления,
 * собираются временные метрики и считается количество draw calls.
 *
 * Диалог отвечает за:
 * - выбор размера тестовой сцены;
 * - выбор режима SingleThreaded или MultiThreaded;
 * - выбор количества рабочих потоков ThreadPool;
 * - запуск и остановку отдельного теста;
 * - запуск полного автоматического benchmark;
 * - отображение прогресса полного benchmark;
 * - отображение итоговой статистики CPU Update, Renderer и draw calls.
 *
 * Связь с остальной системой организована через callbacks. Благодаря этому
 * TestsDialog не зависит от SceneViewport напрямую и остаётся UI-компонентом.
 */
class TestsDialog final : public QDialog {
public:
    /**
     * @brief Создаёт окно тестирования.
     * @param parent Родительский QWidget.
     */
    explicit TestsDialog(QWidget* parent = nullptr);

    /**
     * @brief Устанавливает callback запуска одного теста.
     *
     * Callback получает тестовую сцену, режим обновления и количество workers.
     */
    void SetRunTestCallback(std::function<void(TestScene, ExecutionMode, std::size_t)> callback);

    /**
     * @brief Устанавливает callback остановки текущего теста.
     */
    void SetStopTestCallback(std::function<void()> callback);

    /**
     * @brief Обновляет отображаемую статистику производительности.
     *
     * processing_fps является потенциальной скоростью обработки кадров и
     * вычисляется вне диалога как 1000 / frame_ms. Это не частота обновления окна.
     */
    void SetPerformanceStats(std::size_t object_count, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double processing_fps, double shadow_ms, double main_ms, double editor_ms, std::size_t draw_calls, std::size_t shadow_draw_calls, std::size_t main_draw_calls, std::size_t editor_draw_calls);

    /**
     * @brief Сбрасывает все отображаемые показатели к начальному состоянию.
     */
    void ResetPerformanceStats();

    /**
     * @brief Переключает состояние элементов управления на время теста.
     * @param running true, если benchmark выполняется.
     */
    void SetRunning(bool running);

    /**
     * @brief Устанавливает callback полного автоматического benchmark.
     */
    void SetRunFullBenchmarkCallback(std::function<void()> callback);

    /**
     * @brief Отображает прогресс полного benchmark.
     * @param current Номер текущей конфигурации.
     * @param total Общее количество конфигураций.
     */
    void SetBenchmarkProgress(std::size_t current, std::size_t total);

protected:
    /**
     * @brief Обрабатывает нажатие мыши для перемещения frameless-окна.
     */
    void mousePressEvent(QMouseEvent* event) override;

private:
    /**
     * @brief Возвращает выбранный размер тестовой сцены.
     */
    TestScene GetSelectedTestScene() const;

    /**
     * @brief Возвращает выбранный режим обновления сцены.
     */
    ExecutionMode GetSelectedExecutionMode() const;

    /**
     * @brief Возвращает выбранное количество рабочих потоков.
     *
     * В SingleThreaded режиме всегда возвращается 1.
     */
    std::size_t GetSelectedWorkerCount() const;

    /**
     * @brief Включает или отключает выбор количества workers.
     */
    void UpdateWorkerCountState();

private:
    QComboBox* test_scene_combo_ = nullptr;
    QComboBox* execution_mode_combo_ = nullptr;
    QComboBox* worker_count_combo_ = nullptr;

    QPushButton* run_button_ = nullptr;
    QPushButton* stop_button_ = nullptr;
    QPushButton* full_benchmark_button_ = nullptr;

    QLabel* objects_value_ = nullptr;
    QLabel* workers_value_ = nullptr;
    QLabel* update_value_ = nullptr;
    QLabel* render_value_ = nullptr;
    QLabel* frame_value_ = nullptr;
    QLabel* fps_value_ = nullptr;

    QLabel* shadow_value_ = nullptr;
    QLabel* main_value_ = nullptr;
    QLabel* editor_value_ = nullptr;

    QLabel* draw_calls_value_ = nullptr;
    QLabel* shadow_draw_calls_value_ = nullptr;
    QLabel* main_draw_calls_value_ = nullptr;
    QLabel* editor_draw_calls_value_ = nullptr;

    QLabel* benchmark_progress_value_ = nullptr;

    std::function<void(TestScene, ExecutionMode, std::size_t)> run_test_callback_;
    std::function<void()> stop_test_callback_;
    std::function<void()> run_full_benchmark_callback_;
};
