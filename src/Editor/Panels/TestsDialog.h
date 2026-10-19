#pragma once

#include "Engine/Core/ExecutionMode.h"
#include "Engine/Tests/TestScene.h"

#include <QDialog>

#include <cstddef>
#include <functional>

class QLabel;
class QPushButton;
class QComboBox;
class QMouseEvent;

class TestsDialog : public QDialog {
public:
    explicit TestsDialog(QWidget* parent = nullptr);

    void SetRunTestCallback(std::function<void(TestScene, ExecutionMode, std::size_t)> callback);
    void SetStopTestCallback(std::function<void()> callback);

    void SetPerformanceStats(
        std::size_t object_count,
        std::size_t worker_count,
        double update_ms,
        double render_ms,
        double frame_ms,
        double processing_fps,
        double shadow_ms,
        double main_ms,
        double editor_ms,
        std::size_t draw_calls,
        std::size_t shadow_draw_calls,
        std::size_t main_draw_calls,
        std::size_t editor_draw_calls
    );

    void ResetPerformanceStats();
    void SetRunning(bool running);

    void SetRunFullBenchmarkCallback(std::function<void()> callback);
    void SetBenchmarkProgress(std::size_t current, std::size_t total);

private:
    TestScene GetSelectedTestScene() const;
    ExecutionMode GetSelectedExecutionMode() const;
    std::size_t GetSelectedWorkerCount() const;

    void UpdateWorkerCountState();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QComboBox* test_scene_combo_ = nullptr;
    QComboBox* execution_mode_combo_ = nullptr;
    QComboBox* worker_count_combo_ = nullptr;

    QPushButton* run_button_ = nullptr;
    QPushButton* stop_button_ = nullptr;

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

    std::function<void(TestScene, ExecutionMode, std::size_t)> run_test_callback_;
    std::function<void()> stop_test_callback_;

    QPushButton* full_benchmark_button_ = nullptr;
    QLabel* benchmark_progress_value_ = nullptr;

    std::function<void()> run_full_benchmark_callback_;
};