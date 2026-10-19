#include "Application.h"

#include "../Platform/OpenGL/Glad/glad.h"

#include <GLFW/glfw3.h>

/**
 * @brief Создаёт основной объект приложения Mini Engine.
 *
 * Конструктор инициализирует главное окно приложения
 * размером 1280x720 пикселей с заголовком "Mini Engine".
 *
 * Объект Window создаётся до входа в основной цикл
 * и используется для:
 *
 * - обработки событий GLFW;
 * - проверки запроса на закрытие окна;
 * - получения native GLFWwindow;
 * - обмена front/back buffers.
 */
Application::Application()
    : window_(1280, 720, "Mini Engine") {
}

/**
 * @brief Уничтожает объект Application.
 *
 * Деструктор использует стандартную реализацию.
 *
 * Освобождение ресурсов-членов класса выполняется
 * автоматически в соответствии с их временем жизни.
 */
Application::~Application() = default;

/**
 * @brief Запускает главный цикл приложения.
 *
 * Цикл выполняется до тех пор, пока одновременно выполняются
 * два условия:
 *
 * - флаг is_running_ остаётся true;
 * - окно не получило запрос на закрытие.
 *
 * На каждой итерации выполняется последовательность:
 *
 * 1. обработка событий окна;
 * 2. обновление состояния приложения;
 * 3. рендеринг текущего кадра;
 * 4. обмен front/back buffers.
 *
 * Такая структура соответствует стандартному
 * real-time application loop.
 *
 * @return Код завершения приложения.
 * В текущей реализации при штатном завершении возвращается 0.
 */
int Application::Run() {
    while (is_running_ && !window_.ShouldClose()) {
        window_.PollEvents();

        Update();
        Render();

        window_.SwapBuffers();
    }

    return 0;
}

/**
 * @brief Обновляет состояние приложения.
 *
 * В текущей реализации Update() обрабатывает
 * клавишу Escape.
 *
 * Если Escape находится в состоянии GLFW_PRESS,
 * флаг is_running_ устанавливается в false,
 * после чего основной цикл Run() завершается
 * на следующей проверке условия.
 *
 * Состояние клавиатуры считывается непосредственно
 * через native GLFW window handle.
 */
void Application::Update() {
    if (glfwGetKey(window_.GetNativeHandle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        is_running_ = false;
    }
}

/**
 * @brief Выполняет отрисовку текущего кадра.
 *
 * Метод задаёт цвет очистки framebuffer
 * и очищает:
 *
 * - color buffer;
 * - depth buffer.
 *
 * Текущий цвет фона:
 *
 * R = 0.08
 * G = 0.08
 * B = 0.10
 * A = 1.00
 *
 * После выполнения Render() обмен буферов
 * производится в основном цикле Application::Run()
 * через Window::SwapBuffers().
 */
void Application::Render() {
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}