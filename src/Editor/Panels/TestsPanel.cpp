#include "Editor/Panels/TestsPanel.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <utility>

/**
 * @brief Создаёт панель нагрузочного тестирования.
 */
TestsPanel::TestsPanel(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* root_layout = new QVBoxLayout(this);

    root_layout->setContentsMargins(10, 10, 10, 10);
    root_layout->setSpacing(10);

    /*
     * Выбор тестовой сцены.
     *
     * Пока реально существует только Cube1k.
     * Остальные тесты добавим позже.
     */
    QLabel* test_scene_label = new QLabel("Test Scene", this);

    test_scene_combo_ = new QComboBox(this);
    test_scene_combo_->addItem("1k Cube");

    root_layout->addWidget(test_scene_label);
    root_layout->addWidget(test_scene_combo_);

    /*
     * Выбор режима обработки сцены.
     */
    QLabel* execution_mode_label = new QLabel("Execution Mode", this);

    execution_mode_combo_ = new QComboBox(this);

    execution_mode_combo_->addItem("Single Thread");
    execution_mode_combo_->addItem("Multi Thread");

    root_layout->addWidget(execution_mode_label);
    root_layout->addWidget(execution_mode_combo_);

    /*
     * Управление тестом.
     */
    QHBoxLayout* button_layout = new QHBoxLayout();

    run_button_ = new QPushButton("Run Test", this);
    stop_button_ = new QPushButton("Stop", this);

    stop_button_->setEnabled(false);

    button_layout->addWidget(run_button_);
    button_layout->addWidget(stop_button_);

    root_layout->addLayout(button_layout);

    /*
     * Блок статистики.
     */
    QGroupBox* performance_group = new QGroupBox("Performance", this);

    QFormLayout* performance_layout = new QFormLayout(performance_group);

    objects_value_ = new QLabel("0", performance_group);
    workers_value_ = new QLabel("1", performance_group);
    update_value_ = new QLabel("0.00 ms", performance_group);
    render_value_ = new QLabel("0.00 ms", performance_group);
    frame_value_ = new QLabel("0.00 ms", performance_group);
    fps_value_ = new QLabel("0.0", performance_group);

    performance_layout->addRow("Objects:", objects_value_);
    performance_layout->addRow("Workers:", workers_value_);
    performance_layout->addRow("Update:", update_value_);
    performance_layout->addRow("Render:", render_value_);
    performance_layout->addRow("Frame:", frame_value_);
    performance_layout->addRow("FPS:", fps_value_);

    root_layout->addWidget(performance_group);

    /*
     * Прижимает элементы панели вверх.
     */
    root_layout->addStretch();

    /*
     * TestsPanel ничего не знает о SceneViewport.
     *
     * Он только сообщает наружу, что пользователь
     * нажал кнопку запуска или остановки.
     */
    connect(run_button_, &QPushButton::clicked, this, [this]() {
        if (!run_test_callback_) {
            return;
        }

        run_test_callback_(GetSelectedTestScene(), GetSelectedExecutionMode());
    });

    connect(stop_button_, &QPushButton::clicked, this, [this]() {
        if (!stop_test_callback_) {
            return;
        }

        stop_test_callback_();
    });
}

void TestsPanel::SetRunTestCallback(std::function<void(TestScene, ExecutionMode)> callback) {
    run_test_callback_ = std::move(callback);
}

void TestsPanel::SetStopTestCallback(std::function<void()> callback) {
    stop_test_callback_ = std::move(callback);
}

void TestsPanel::SetPerformanceStats(std::size_t object_count, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double fps) {
    objects_value_->setText(QString::number(object_count));
    workers_value_->setText(QString::number(worker_count));

    update_value_->setText(QString::number(update_ms, 'f', 2) + " ms");
    render_value_->setText(QString::number(render_ms, 'f', 2) + " ms");
    frame_value_->setText(QString::number(frame_ms, 'f', 2) + " ms");

    fps_value_->setText(QString::number(fps, 'f', 1));
}

void TestsPanel::ResetPerformanceStats() {
    objects_value_->setText("0");
    workers_value_->setText("1");

    update_value_->setText("0.00 ms");
    render_value_->setText("0.00 ms");
    frame_value_->setText("0.00 ms");

    fps_value_->setText("0.0");
}

void TestsPanel::SetRunning(bool running) {
    run_button_->setEnabled(!running);
    stop_button_->setEnabled(running);

    test_scene_combo_->setEnabled(!running);
    execution_mode_combo_->setEnabled(!running);
}

TestScene TestsPanel::GetSelectedTestScene() const {
    switch (test_scene_combo_->currentIndex()) {
        case 0:
            return TestScene::Cube1k;

        default:
            return TestScene::Cube1k;
    }
}

ExecutionMode TestsPanel::GetSelectedExecutionMode() const {
    switch (execution_mode_combo_->currentIndex()) {
        case 1:
            return ExecutionMode::MultiThreaded;

        case 0:
        default:
            return ExecutionMode::SingleThreaded;
    }
}