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
 * @brief Отдельное окно нагрузочных тестов Mini Engine.
 *
 * Окно не является DockWidget и не изменяет layout редактора.
 * Используется собственная тёмная верхняя панель вместо
 * стандартного белого title bar macOS.
 */
class TestsDialog final : public QDialog {
public:
	explicit TestsDialog(QWidget* parent = nullptr);

	void SetRunTestCallback(std::function<void(TestScene, ExecutionMode)> callback);
	void SetStopTestCallback(std::function<void()> callback);

	void SetPerformanceStats(std::size_t object_count, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double fps);
	void ResetPerformanceStats();
	void SetRunning(bool running);

protected:
	void mousePressEvent(QMouseEvent* event) override;

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