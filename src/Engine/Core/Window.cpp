#include "Window.h"
#include "../Platform/OpenGL/Glad/glad.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

/**
 * @brief Создаёт окно GLFW и инициализирует OpenGL-контекст.
 *
 * Конструктор выполняет полную подготовку standalone-окна Mini Engine
 * к последующему OpenGL-рендерингу.
 *
 * Последовательность инициализации:
 *
 * 1. Инициализируется библиотека GLFW.
 * 2. Задаются параметры OpenGL-контекста версии 3.3 Core Profile.
 * 3. На macOS дополнительно включается Forward Compatible Context.
 * 4. Создаётся GLFWwindow.
 * 5. Созданный OpenGL-контекст делается текущим.
 * 6. Через GLAD загружаются адреса OpenGL-функций.
 * 7. Настраивается начальный viewport.
 * 8. Регистрируется callback изменения framebuffer.
 * 9. Включается depth testing.
 * 10. В консоль выводится фактическая версия OpenGL.
 *
 * @param width Начальная ширина окна в пикселях.
 * @param height Начальная высота окна в пикселях.
 * @param title Заголовок окна.
 *
 * @throws std::runtime_error Если не удалось инициализировать GLFW,
 * создать GLFWwindow или инициализировать GLAD.
 */
Window::Window(int width, int height, const char* title)
    : width_(width),
      height_(height) {
    /**
     * GLFW должен быть инициализирован
     * до создания окна и OpenGL-контекста.
     */
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

#ifdef __APPLE__
    /**
     * На macOS для использования OpenGL Core Profile
     * требуется forward-compatible context.
     */
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    /**
     * Mini Engine запрашивает OpenGL 3.3 Core Profile.
     */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /**
     * Создаём GLFWwindow.
     *
     * Последние два nullptr означают:
     *
     * - окно не создаётся в полноэкранном режиме;
     * - OpenGL-контекст не разделяется с другим окном.
     */
    handle_ = glfwCreateWindow(width_, height_, title, nullptr, nullptr);

    if (!handle_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    /**
     * Созданный OpenGL-контекст необходимо сделать текущим
     * перед загрузкой OpenGL-функций и выполнением GL-команд.
     */
    glfwMakeContextCurrent(handle_);

    /**
     * GLAD получает адреса OpenGL-функций
     * через glfwGetProcAddress().
     */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(handle_);
        glfwTerminate();

        throw std::runtime_error("Failed to initialize GLAD");
    }

    /**
     * Начальный OpenGL viewport соответствует
     * размерам framebuffer создаваемого окна.
     */
    glViewport(0, 0, width_, height_);

    /**
     * GLFW callback является C-style callback
     * и не получает указатель на объект Window автоматически.
     *
     * Поэтому this сохраняется внутри GLFWwindow
     * через glfwSetWindowUserPointer().
     *
     * Callback затем получает его обратно
     * через glfwGetWindowUserPointer().
     */
    glfwSetWindowUserPointer(handle_, this);

    /**
     * Callback вызывается GLFW
     * при изменении размера framebuffer.
     *
     * Внутри обновляются:
     *
     * - width_;
     * - height_;
     * - OpenGL viewport.
     *
     * Используются размеры framebuffer, а не логический размер окна,
     * что особенно важно на дисплеях с высоким DPI.
     */
    glfwSetFramebufferSizeCallback(
        handle_,
        [](GLFWwindow* window, int new_width, int new_height) {
            auto* self = static_cast<Window*>(
                glfwGetWindowUserPointer(window)
            );

            self->width_ = new_width;
            self->height_ = new_height;

            glViewport(0, 0, new_width, new_height);
        }
    );

    /**
     * Включаем depth testing.
     *
     * OpenGL будет использовать depth buffer,
     * чтобы определять видимость фрагментов
     * относительно расстояния до камеры.
     */
    glEnable(GL_DEPTH_TEST);

    /**
     * Выводим фактически созданную версию OpenGL.
     *
     * Это позволяет проверить,
     * какой контекст был предоставлен системой.
     */
    std::cout
        << "OpenGL version: "
        << glGetString(GL_VERSION)
        << std::endl;
}

/**
 * @brief Освобождает GLFWwindow и завершает работу GLFW.
 *
 * Если окно существует, оно сначала уничтожается
 * через glfwDestroyWindow().
 *
 * После этого GLFW завершается вызовом glfwTerminate().
 */
Window::~Window() {
    if (handle_) {
        glfwDestroyWindow(handle_);
        handle_ = nullptr;
    }

    glfwTerminate();
}

/**
 * Ранее использовавшиеся callback-функции.
 *
 * Они оставлены в исходном коде в закомментированном виде
 * и в текущей реализации не вызываются.
 *
 * Обработка Escape сейчас выполняется в Application::Update(),
 * а изменение viewport — через lambda,
 * зарегистрированную непосредственно в конструкторе Window.
 *
 * void Window::processInput(GLFWwindow* window) {
 *     if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
 *         glfwSetWindowShouldClose(window, true);
 * }
 *
 * void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
 *     // Подстройка viewport под изменение framebuffer.
 *     glViewport(0, 0, width, height);
 * }
 */

/**
 * @brief Проверяет, запрошено ли закрытие окна.
 *
 * Метод передаёт запрос непосредственно GLFW
 * через glfwWindowShouldClose().
 *
 * @return true, если окно должно быть закрыто;
 * иначе false.
 */
bool Window::ShouldClose() const {
    return glfwWindowShouldClose(handle_);
}

/**
 * @brief Обрабатывает накопившиеся события GLFW.
 *
 * glfwPollEvents() обрабатывает события операционной системы,
 * включая:
 *
 * - клавиатуру;
 * - мышь;
 * - изменение размера окна;
 * - запрос на закрытие окна.
 */
void Window::PollEvents() {
    glfwPollEvents();
}

/**
 * @brief Меняет front и back buffers окна.
 *
 * Метод вызывается после завершения рендеринга кадра.
 *
 * Сформированный back buffer становится видимым пользователю,
 * после чего OpenGL может формировать следующий кадр.
 */
void Window::SwapBuffers() {
    glfwSwapBuffers(handle_);
}

/**
 * @brief Возвращает текущую ширину framebuffer.
 *
 * Значение обновляется callback-функцией GLFW
 * при изменении размера framebuffer.
 *
 * @return Текущая ширина в пикселях.
 */
int Window::GetWidth() const {
    return width_;
}

/**
 * @brief Возвращает текущую высоту framebuffer.
 *
 * Значение обновляется callback-функцией GLFW
 * при изменении размера framebuffer.
 *
 * @return Текущая высота в пикселях.
 */
int Window::GetHeight() const {
    return height_;
}

/**
 * @brief Вычисляет текущее соотношение сторон framebuffer.
 *
 * Aspect ratio определяется как:
 *
 * width / height.
 *
 * Если высота равна нулю,
 * возвращается 1.0f, чтобы избежать деления на ноль.
 *
 * @return Текущее соотношение ширины к высоте.
 */
float Window::GetAspectRatio() const {
    if (height_ == 0) {
        return 1.0f;
    }

    return static_cast<float>(width_) / static_cast<float>(height_);
}

/**
 * @brief Возвращает native GLFWwindow.
 *
 * Метод предоставляет доступ к GLFW handle
 * для частей приложения, которым необходимо напрямую
 * использовать GLFW API.
 *
 * Например, Application использует этот указатель
 * для проверки состояния клавиатуры.
 *
 * @return Указатель на принадлежащий Window объект GLFWwindow.
 */
GLFWwindow* Window::GetNativeHandle() {
    return handle_;
}