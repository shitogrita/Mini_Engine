#include "Editor/Panels/TestsDialog.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWindow>

#include <utility>

TestsDialog::TestsDialog(QWidget* parent) : QDialog(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);

    setObjectName("TestsDialog");
    setWindowTitle("Tests");

    resize(460, 590);
    setMinimumSize(430, 560);

    setStyleSheet(
        R"(
            QDialog#TestsDialog {
                background-color: #3c3f41;
                border: 1px solid #585c61;
            }

            QLabel {
                color: #d7dae0;
                background-color: transparent;
                font-size: 13px;
            }

            QLabel#TestsTitle {
                color: #f0f0f0;
                font-size: 15px;
                font-weight: 600;
            }

            QLabel#PerformanceName {
                color: #d7dae0;
                font-size: 13px;
            }

            QLabel#PerformanceValue {
                color: #e2e5e9;
                font-size: 13px;
                font-weight: 500;
            }

            QGroupBox {
                background-color: #3c3f41;
                color: #d7dae0;
                border: 1px solid #55595f;
                border-radius: 5px;
                margin-top: 14px;
                padding-top: 12px;
                font-weight: 600;
            }

            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                left: 10px;
                padding: 0px 5px;
                color: #d7dae0;
                background-color: #3c3f41;
            }

            QComboBox {
                background-color: #2b2d30;
                color: #d7dae0;
                border: 1px solid #55595f;
                border-radius: 4px;
                padding: 6px 8px;
                min-height: 26px;
            }

            QComboBox:hover {
                border: 1px solid #6a6d72;
            }

            QComboBox:focus {
                border: 1px solid #4a88c7;
            }

            QComboBox::drop-down {
                border: none;
                width: 24px;
            }

            QComboBox QAbstractItemView {
                background-color: #313335;
                color: #d7dae0;
                border: 1px solid #55595f;
                selection-background-color: #365880;
                selection-color: #ffffff;
                outline: none;
            }

            QPushButton {
                background-color: #45484c;
                color: #d7dae0;
                border: 1px solid #5b5f64;
                border-radius: 4px;
                padding: 7px 14px;
                min-height: 27px;
            }

            QPushButton:hover {
                background-color: #50545a;
                border-color: #6a6d72;
            }

            QPushButton:pressed {
                background-color: #365880;
                color: #ffffff;
            }

            QPushButton:disabled {
                color: #777a80;
                background-color: #35373a;
                border-color: #484b50;
            }

            QPushButton#TestsCloseButton {
                background-color: #55595f;
                border: none;
                color: #ffffff;
                font-size: 18px;
                padding: 0px;
            }

            QPushButton#TestsCloseButton:hover {
                background-color: #656970;
            }
        )"
    );

    QVBoxLayout* root_layout = new QVBoxLayout(this);

    root_layout->setContentsMargins(14, 10, 14, 14);
    root_layout->setSpacing(14);

    QHBoxLayout* title_layout = new QHBoxLayout();

    title_layout->setContentsMargins(4, 0, 0, 0);
    title_layout->setSpacing(8);

    QLabel* title_label = new QLabel("Tests", this);
    title_label->setObjectName("TestsTitle");

    QPushButton* close_button = new QPushButton("×", this);
    close_button->setObjectName("TestsCloseButton");
    close_button->setFixedSize(30, 30);

    title_layout->addWidget(title_label);
    title_layout->addStretch();
    title_layout->addWidget(close_button);

    root_layout->addLayout(title_layout);

    QGroupBox* settings_group = new QGroupBox("Test Settings", this);
    settings_group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    settings_group->setMinimumHeight(145);

    QFormLayout* settings_layout = new QFormLayout(settings_group);

    settings_layout->setContentsMargins(20, 22, 20, 18);
    settings_layout->setHorizontalSpacing(18);
    settings_layout->setVerticalSpacing(12);
    settings_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    test_scene_combo_ = new QComboBox(settings_group);
    test_scene_combo_->addItem("1k Cube");

    execution_mode_combo_ = new QComboBox(settings_group);
    execution_mode_combo_->addItem("Single Thread");
    execution_mode_combo_->addItem("Multi Thread");

    test_scene_combo_->setMinimumWidth(150);
    execution_mode_combo_->setMinimumWidth(180);

    settings_layout->addRow("Test Scene:", test_scene_combo_);
    settings_layout->addRow("Execution Mode:", execution_mode_combo_);

    root_layout->addWidget(settings_group);

    QHBoxLayout* button_layout = new QHBoxLayout();

    button_layout->setSpacing(10);
    button_layout->setContentsMargins(0, 0, 0, 0);

    run_button_ = new QPushButton("Run Test", this);
    stop_button_ = new QPushButton("Stop", this);

    run_button_->setMinimumWidth(105);
    stop_button_->setMinimumWidth(80);

    stop_button_->setEnabled(false);

    button_layout->addWidget(run_button_);
    button_layout->addWidget(stop_button_);
    button_layout->addStretch();

    root_layout->addLayout(button_layout);

    QGroupBox* performance_group = new QGroupBox("Performance", this);

    performance_group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    performance_group->setMinimumHeight(245);

    QGridLayout* performance_layout = new QGridLayout(performance_group);

    performance_layout->setContentsMargins(32, 28, 32, 22);
    performance_layout->setHorizontalSpacing(26);
    performance_layout->setVerticalSpacing(6);
    performance_layout->setColumnStretch(0, 1);
    performance_layout->setColumnStretch(1, 1);

    auto create_name_label = [performance_group](const QString& text) {
        QLabel* label = new QLabel(text, performance_group);
        label->setObjectName("PerformanceName");
        label->setMinimumHeight(26);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return label;
    };

    auto configure_value_label = [](QLabel* label) {
        label->setObjectName("PerformanceValue");
        label->setMinimumHeight(26);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    };

    objects_value_ = new QLabel("0", performance_group);
    workers_value_ = new QLabel("1", performance_group);
    update_value_ = new QLabel("0.00 ms", performance_group);
    render_value_ = new QLabel("0.00 ms", performance_group);
    frame_value_ = new QLabel("0.00 ms", performance_group);
    fps_value_ = new QLabel("0.0", performance_group);

    configure_value_label(objects_value_);
    configure_value_label(workers_value_);
    configure_value_label(update_value_);
    configure_value_label(render_value_);
    configure_value_label(frame_value_);
    configure_value_label(fps_value_);

    performance_layout->addWidget(create_name_label("Objects:"), 0, 0);
    performance_layout->addWidget(objects_value_, 0, 1);

    performance_layout->addWidget(create_name_label("Workers:"), 1, 0);
    performance_layout->addWidget(workers_value_, 1, 1);

    performance_layout->addWidget(create_name_label("Update:"), 2, 0);
    performance_layout->addWidget(update_value_, 2, 1);

    performance_layout->addWidget(create_name_label("Render:"), 3, 0);
    performance_layout->addWidget(render_value_, 3, 1);

    performance_layout->addWidget(create_name_label("Frame:"), 4, 0);
    performance_layout->addWidget(frame_value_, 4, 1);

    performance_layout->addWidget(create_name_label("FPS:"), 5, 0);
    performance_layout->addWidget(fps_value_, 5, 1);

    for (int row = 0; row < 6; ++row) {
        performance_layout->setRowMinimumHeight(row, 28);
    }

    root_layout->addWidget(performance_group);

    connect(close_button, &QPushButton::clicked, this, &QWidget::hide);

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

void TestsDialog::SetRunTestCallback(std::function<void(TestScene, ExecutionMode)> callback) {
    run_test_callback_ = std::move(callback);
}

void TestsDialog::SetStopTestCallback(std::function<void()> callback) {
    stop_test_callback_ = std::move(callback);
}

void TestsDialog::SetPerformanceStats(std::size_t object_count, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double fps) {
    objects_value_->setText(QString::number(object_count));
    workers_value_->setText(QString::number(worker_count));

    update_value_->setText(QString::number(update_ms, 'f', 2) + " ms");
    render_value_->setText(QString::number(render_ms, 'f', 2) + " ms");
    frame_value_->setText(QString::number(frame_ms, 'f', 2) + " ms");
    fps_value_->setText(QString::number(fps, 'f', 1));
}

void TestsDialog::ResetPerformanceStats() {
    objects_value_->setText("0");
    workers_value_->setText("1");
    update_value_->setText("0.00 ms");
    render_value_->setText("0.00 ms");
    frame_value_->setText("0.00 ms");
    fps_value_->setText("0.0");
}

void TestsDialog::SetRunning(bool running) {
    run_button_->setEnabled(!running);
    stop_button_->setEnabled(running);

    test_scene_combo_->setEnabled(!running);
    execution_mode_combo_->setEnabled(!running);
}

void TestsDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->position().y() <= 42.0) {
        if (windowHandle()) {
            windowHandle()->startSystemMove();
        }

        event->accept();
        return;
    }

    QDialog::mousePressEvent(event);
}

TestScene TestsDialog::GetSelectedTestScene() const {
    switch (test_scene_combo_->currentIndex()) {
        case 0:
        default:
            return TestScene::Cube1k;
    }
}

ExecutionMode TestsDialog::GetSelectedExecutionMode() const {
    switch (execution_mode_combo_->currentIndex()) {
        case 1:
            return ExecutionMode::MultiThreaded;

        case 0:
        default:
            return ExecutionMode::SingleThreaded;
    }
}