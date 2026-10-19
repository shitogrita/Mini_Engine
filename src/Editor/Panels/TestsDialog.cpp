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

#include <cstddef>
#include <thread>
#include <utility>

/**
 * @brief Создаёт окно управления нагрузочными тестами Mini Engine.
 *
 * Диалог позволяет выбрать размер тестовой сцены, режим обновления,
 * число рабочих потоков и запускать как отдельный тест, так и полный benchmark.
 *
 * @param parent Родительское окно Qt.
 */
TestsDialog::TestsDialog(QWidget* parent) : QDialog(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);

    setObjectName("TestsDialog");
    setWindowTitle("Tests");

    resize(660, 620);
    setMinimumSize(620, 590);

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

            QComboBox:disabled {
                color: #777a80;
                background-color: #35373a;
                border-color: #484b50;
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
    settings_group->setMinimumHeight(185);

    QFormLayout* settings_layout = new QFormLayout(settings_group);

    settings_layout->setContentsMargins(20, 22, 20, 18);
    settings_layout->setHorizontalSpacing(18);
    settings_layout->setVerticalSpacing(12);
    settings_layout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    /*
     * Выбор размера тестовой сцены.
     */
    test_scene_combo_ = new QComboBox(settings_group);

    test_scene_combo_->addItem("1 000 Cubes");
    test_scene_combo_->addItem("10 000 Cubes");
    test_scene_combo_->addItem("25 000 Cubes");
    test_scene_combo_->addItem("50 000 Cubes");

    /*
     * Выбор способа выполнения CPU Scene Update.
     */
    execution_mode_combo_ = new QComboBox(settings_group);

    execution_mode_combo_->addItem("Single Thread");
    execution_mode_combo_->addItem("Multi Thread");

    /*
     * Выбор количества worker-потоков ThreadPool.
     *
     * Это значение используется только в MultiThreaded режиме.
     */
    worker_count_combo_ = new QComboBox(settings_group);

    /*
     * hardware_concurrency() сообщает рекомендуемое количество
     * аппаратных потоков, доступных приложению.
     *
     * Стандарт разрешает вернуть 0,
     * если значение определить невозможно.
     */
    std::size_t hardware_worker_count = std::thread::hardware_concurrency();

    if (hardware_worker_count == 0) {
        hardware_worker_count = 1;
    }

    /*
     * Базовые значения нужны для исследования масштабирования:
     *
     * 1 worker
     * 2 workers
     * 4 workers
     * 6 workers
     * 8 workers
     *
     * Значения выше возможностей текущего процессора
     * в ComboBox не добавляются.
     */
    const std::size_t worker_options[] = {
        1,
        2,
        4,
        6,
        8
    };

    for (const std::size_t worker_count : worker_options) {
        if (worker_count > hardware_worker_count) {
            continue;
        }

        worker_count_combo_->addItem(
            QString::number(static_cast<qulonglong>(worker_count)),
            static_cast<qulonglong>(worker_count)
        );
    }

    /*
     * Максимальное количество hardware threads добавляем отдельно.
     *
     * Например, если hardware_concurrency() == 10,
     * итоговый список будет:
     *
     * 1
     * 2
     * 4
     * 6
     * 8
     * 10
     */
    bool hardware_worker_count_exists = false;

    for (int index = 0; index < worker_count_combo_->count(); ++index) {
        const std::size_t existing_worker_count =
            static_cast<std::size_t>(
                worker_count_combo_->itemData(index).toULongLong()
            );

        if (existing_worker_count == hardware_worker_count) {
            hardware_worker_count_exists = true;
            break;
        }
    }

    if (!hardware_worker_count_exists) {
        worker_count_combo_->addItem(
            QString::number(static_cast<qulonglong>(hardware_worker_count)),
            static_cast<qulonglong>(hardware_worker_count)
        );
    }

    /*
     * По умолчанию MultiThread benchmark использует
     * максимальное доступное количество workers.
     */
    worker_count_combo_->setCurrentIndex(
        worker_count_combo_->count() - 1
    );

    test_scene_combo_->setMinimumWidth(180);
    execution_mode_combo_->setMinimumWidth(180);
    worker_count_combo_->setMinimumWidth(180);

    settings_layout->addRow("Test Scene:", test_scene_combo_);
    settings_layout->addRow("Execution Mode:", execution_mode_combo_);
    settings_layout->addRow("Workers:", worker_count_combo_);

    root_layout->addWidget(settings_group);

    QHBoxLayout* button_layout = new QHBoxLayout();

    button_layout->setSpacing(10);
    button_layout->setContentsMargins(0, 0, 0, 0);

    run_button_ = new QPushButton("Run Test", this);
    full_benchmark_button_ = new QPushButton("Run Full Benchmark", this);
    stop_button_ = new QPushButton("Stop", this);

    run_button_->setMinimumWidth(105);
    full_benchmark_button_->setMinimumWidth(150);
    stop_button_->setMinimumWidth(80);

    stop_button_->setEnabled(false);

    button_layout->addWidget(run_button_);
    button_layout->addWidget(full_benchmark_button_);
    button_layout->addWidget(stop_button_);
    button_layout->addStretch();

    benchmark_progress_value_ = new QLabel("Full Benchmark: idle", this);

    benchmark_progress_value_->setAlignment(
        Qt::AlignLeft | Qt::AlignVCenter
    );

    root_layout->addWidget(
        benchmark_progress_value_
    );

    root_layout->addLayout(button_layout);

    QGroupBox* performance_group = new QGroupBox("Performance", this);

    performance_group->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
    );

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

        label->setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Fixed
        );

        return label;
    };

    auto configure_value_label = [](QLabel* label) {
        label->setObjectName("PerformanceValue");
        label->setMinimumHeight(26);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        label->setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Fixed
        );
    };

    objects_value_ = new QLabel("0", performance_group);
    workers_value_ = new QLabel("1", performance_group);
    update_value_ = new QLabel("0.00 ms", performance_group);
    render_value_ = new QLabel("0.00 ms", performance_group);
    frame_value_ = new QLabel("0.00 ms", performance_group);
    fps_value_ = new QLabel("0.0", performance_group);

    shadow_value_ = new QLabel("0.00 ms", performance_group);
    main_value_ = new QLabel("0.00 ms", performance_group);
    editor_value_ = new QLabel("0.00 ms", performance_group);

    draw_calls_value_ = new QLabel("0", performance_group);
    shadow_draw_calls_value_ = new QLabel("0", performance_group);
    main_draw_calls_value_ = new QLabel("0", performance_group);
    editor_draw_calls_value_ = new QLabel("0", performance_group);

    configure_value_label(objects_value_);
    configure_value_label(workers_value_);
    configure_value_label(update_value_);
    configure_value_label(render_value_);
    configure_value_label(frame_value_);
    configure_value_label(fps_value_);

    configure_value_label(shadow_value_);
    configure_value_label(main_value_);
    configure_value_label(editor_value_);

    configure_value_label(draw_calls_value_);
    configure_value_label(shadow_draw_calls_value_);
    configure_value_label(main_draw_calls_value_);
    configure_value_label(editor_draw_calls_value_);

    performance_layout->setColumnStretch(0, 1);
    performance_layout->setColumnStretch(1, 1);
    performance_layout->setColumnStretch(2, 1);
    performance_layout->setColumnStretch(3, 1);

    performance_layout->addWidget(create_name_label("Objects:"), 0, 0);
    performance_layout->addWidget(objects_value_, 0, 1);

    performance_layout->addWidget(create_name_label("Shadow:"), 0, 2);
    performance_layout->addWidget(shadow_value_, 0, 3);

    performance_layout->addWidget(create_name_label("Workers:"), 1, 0);
    performance_layout->addWidget(workers_value_, 1, 1);

    performance_layout->addWidget(create_name_label("Main:"), 1, 2);
    performance_layout->addWidget(main_value_, 1, 3);

    performance_layout->addWidget(create_name_label("Update:"), 2, 0);
    performance_layout->addWidget(update_value_, 2, 1);

    performance_layout->addWidget(create_name_label("Editor:"), 2, 2);
    performance_layout->addWidget(editor_value_, 2, 3);

    performance_layout->addWidget(create_name_label("Render:"), 3, 0);
    performance_layout->addWidget(render_value_, 3, 1);

    performance_layout->addWidget(create_name_label("Draw Calls:"), 3, 2);
    performance_layout->addWidget(draw_calls_value_, 3, 3);

    performance_layout->addWidget(create_name_label("Frame:"), 4, 0);
    performance_layout->addWidget(frame_value_, 4, 1);

    performance_layout->addWidget(create_name_label("Shadow Draws:"), 4, 2);
    performance_layout->addWidget(shadow_draw_calls_value_, 4, 3);

    performance_layout->addWidget(create_name_label("Processing FPS:"), 5, 0);
    performance_layout->addWidget(fps_value_, 5, 1);

    performance_layout->addWidget(create_name_label("Main Draws:"), 5, 2);
    performance_layout->addWidget(main_draw_calls_value_, 5, 3);

    performance_layout->addWidget(create_name_label("Editor Draws:"), 6, 2);
    performance_layout->addWidget(editor_draw_calls_value_, 6, 3);

    for (int row = 0; row < 7; ++row) {
        performance_layout->setRowMinimumHeight(row, 28);
    }

    root_layout->addWidget(performance_group);

    /*
     * Закрытие окна Tests только скрывает окно.
     *
     * Сам TestsDialog остаётся существовать,
     * поэтому его не приходится создавать заново.
     */
    connect(
        close_button,
        &QPushButton::clicked,
        this,
        &QWidget::hide
    );

    /*
     * При изменении Execution Mode состояние Workers
     * также необходимо обновить.
     *
     * В Single Thread выбирать количество workers бессмысленно.
     */
    connect(
        execution_mode_combo_,
        &QComboBox::currentIndexChanged,
        this,
        [this](int) {
            UpdateWorkerCountState();
        }
    );

    /*
     * Передаём выбранные параметры benchmark
     * владельцу TestsDialog.
     */
    connect(
        run_button_,
        &QPushButton::clicked,
        this,
        [this]() {
            if (!run_test_callback_) {
                return;
            }

            run_test_callback_(
                GetSelectedTestScene(),
                GetSelectedExecutionMode(),
                GetSelectedWorkerCount()
            );
        }
    );

    connect(
        full_benchmark_button_,
        &QPushButton::clicked,
        this,
        [this]() {
            if (!run_full_benchmark_callback_) {
                return;
            }

            run_full_benchmark_callback_();
        }
    );

    connect(
        stop_button_,
        &QPushButton::clicked,
        this,
        [this]() {
            if (!stop_test_callback_) {
                return;
            }

            stop_test_callback_();
        }
    );

    /*
     * При открытии окна по умолчанию выбран Single Thread,
     * поэтому Workers должен быть disabled.
     */
    UpdateWorkerCountState();
}

/**
 * @brief Устанавливает callback запуска одного теста.
 *
 * @param callback Функция, получающая тестовую сцену, режим выполнения и число workers.
 */
void TestsDialog::SetRunTestCallback(std::function<void(TestScene, ExecutionMode, std::size_t)> callback) {
    run_test_callback_ = std::move(callback);
}

/**
 * @brief Устанавливает callback остановки текущего теста.
 *
 * @param callback Функция остановки benchmark.
 */
void TestsDialog::SetStopTestCallback(std::function<void()> callback) {
    stop_test_callback_ = std::move(callback);
}

/**
 * @brief Обновляет отображаемые показатели производительности.
 *
 * Все значения передаются уже рассчитанными системой benchmark. Диалог
 * только форматирует их и отображает пользователю.
 */
void TestsDialog::SetPerformanceStats(std::size_t object_count, std::size_t worker_count, double update_ms, double render_ms, double frame_ms, double processing_fps, double shadow_ms, double main_ms, double editor_ms, std::size_t draw_calls, std::size_t shadow_draw_calls, std::size_t main_draw_calls, std::size_t editor_draw_calls) {
    objects_value_->setText(
        QString::number(
            static_cast<qulonglong>(
                object_count
            )
        )
    );

    workers_value_->setText(
        QString::number(
            static_cast<qulonglong>(
                worker_count
            )
        )
    );

    update_value_->setText(
        QString::number(
            update_ms,
            'f',
            2
        ) + " ms"
    );

    render_value_->setText(
        QString::number(
            render_ms,
            'f',
            2
        ) + " ms"
    );

    frame_value_->setText(
        QString::number(
            frame_ms,
            'f',
            2
        ) + " ms"
    );

    fps_value_->setText(
        QString::number(
            processing_fps,
            'f',
            1
        )
    );

    shadow_value_->setText(
        QString::number(
            shadow_ms,
            'f',
            2
        ) + " ms"
    );

    main_value_->setText(
        QString::number(
            main_ms,
            'f',
            2
        ) + " ms"
    );

    editor_value_->setText(
        QString::number(
            editor_ms,
            'f',
            2
        ) + " ms"
    );

    draw_calls_value_->setText(
        QString::number(
            static_cast<qulonglong>(
                draw_calls
            )
        )
    );

    shadow_draw_calls_value_->setText(
        QString::number(
            static_cast<qulonglong>(
                shadow_draw_calls
            )
        )
    );

    main_draw_calls_value_->setText(
        QString::number(
            static_cast<qulonglong>(
                main_draw_calls
            )
        )
    );

    editor_draw_calls_value_->setText(
        QString::number(
            static_cast<qulonglong>(
                editor_draw_calls
            )
        )
    );
}

/**
 * @brief Сбрасывает визуальные значения статистики к начальному состоянию.
 */
void TestsDialog::ResetPerformanceStats() {
    objects_value_->setText("0");
    workers_value_->setText("1");

    update_value_->setText("0.00 ms");
    render_value_->setText("0.00 ms");
    frame_value_->setText("0.00 ms");
    fps_value_->setText("0.0");

    shadow_value_->setText("0.00 ms");
    main_value_->setText("0.00 ms");
    editor_value_->setText("0.00 ms");

    draw_calls_value_->setText("0");
    shadow_draw_calls_value_->setText("0");
    main_draw_calls_value_->setText("0");
    editor_draw_calls_value_->setText("0");
}

/**
 * @brief Переключает элементы управления между состояниями idle и running.
 *
 * Во время теста изменение конфигурации блокируется, а кнопка Stop включается.
 */
void TestsDialog::SetRunning(bool running) {
    run_button_->setEnabled(!running);
    full_benchmark_button_->setEnabled(!running);

    stop_button_->setEnabled(running);

    test_scene_combo_->setEnabled(!running);
    execution_mode_combo_->setEnabled(!running);

    if (running) {
        worker_count_combo_->setEnabled(false);
    } else {
        UpdateWorkerCountState();
    }
}

/**
 * @brief Обновляет доступность выбора количества workers.
 *
 * В SingleThreaded режиме ThreadPool не используется, поэтому выбор workers отключён.
 */
void TestsDialog::UpdateWorkerCountState() {
    if (!worker_count_combo_) {
        return;
    }

    const bool multi_threaded =
        GetSelectedExecutionMode() == ExecutionMode::MultiThreaded;

    worker_count_combo_->setEnabled(
        multi_threaded
    );
}

/**
 * @brief Позволяет перемещать frameless-окно за верхнюю область заголовка.
 */
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

/**
 * @brief Возвращает тестовую сцену, выбранную пользователем.
 */
TestScene TestsDialog::GetSelectedTestScene() const {
    switch (test_scene_combo_->currentIndex()) {
        case 1:
            return TestScene::Cube10k;

        case 2:
            return TestScene::Cube25k;

        case 3:
            return TestScene::Cube50k;

        case 0:
        default:
            return TestScene::Cube1k;
    }
}

/**
 * @brief Возвращает выбранный режим обновления сцены.
 */
ExecutionMode TestsDialog::GetSelectedExecutionMode() const {
    switch (execution_mode_combo_->currentIndex()) {
        case 1:
            return ExecutionMode::MultiThreaded;

        case 0:
        default:
            return ExecutionMode::SingleThreaded;
    }
}

/**
 * @brief Возвращает выбранное количество рабочих потоков.
 *
 * Для SingleThreaded режима всегда возвращается 1.
 */
std::size_t TestsDialog::GetSelectedWorkerCount() const {
    /*
     * SingleThreaded выполняет Scene Update
     * непосредственно в текущем потоке.
     *
     * ThreadPool в этом режиме не участвует.
     */
    if (GetSelectedExecutionMode() == ExecutionMode::SingleThreaded) {
        return 1;
    }

    if (!worker_count_combo_) {
        return 1;
    }

    const qulonglong selected_worker_count =
        worker_count_combo_->currentData().toULongLong();

    if (selected_worker_count == 0) {
        return 1;
    }

    return static_cast<std::size_t>(
        selected_worker_count
    );
}


/**
 * @brief Устанавливает callback запуска полного benchmark.
 */
void TestsDialog::SetRunFullBenchmarkCallback(std::function<void()> callback) {
    run_full_benchmark_callback_ = std::move(callback);
}

/**
 * @brief Обновляет строку прогресса полного benchmark.
 *
 * @param current Номер текущей конфигурации.
 * @param total Общее количество конфигураций.
 */
void TestsDialog::SetBenchmarkProgress(std::size_t current, std::size_t total) {
    benchmark_progress_value_->setText(
        "Full Benchmark: " +
        QString::number(static_cast<qulonglong>(current)) +
        " / " +
        QString::number(static_cast<qulonglong>(total))
    );
}
