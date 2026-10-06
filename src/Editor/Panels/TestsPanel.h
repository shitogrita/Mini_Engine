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
 * @brief Панель запуска нагрузочных тестов Mini Engine.
 *
 * TestsPanel отвечает только за интерфейс:
 * - выбор тестовой сцены;
 * - выбор режима выполнения;
 * - запуск и остановку теста;
 * - отображение статистики производительности.
 *
 * Сама панель не создаёт SceneObject и не выполняет benchmark.
 */
class TestsPanel final : public QWidget {
public:
	explicit TestsPanel(QWidget* parent = nullptr);

	void SetRunTestCallback(std::function<void(TestScene, ExecutionMode)> callback);
	void SetStopTestCallback(std::function<void()> callback);

	void SetPerformanceStats(std::size_t object_count, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double fps);
	void ResetPerformanceStats();

	void SetRunning(bool running);

private:
	TestScene GetSelectedTestScene() const;
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