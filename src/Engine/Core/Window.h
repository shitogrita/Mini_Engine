#pragma once

struct GLFWwindow;

/**
 * @brief Управляет GLFW-окном и связанным с ним OpenGL-контекстом.
 *
 * Window является обёрткой над GLFWwindow и предоставляет
 * основной интерфейс для работы standalone-приложения Mini Engine
 * с оконной системой.
 *
 * Класс отвечает за:
 *
 * - создание GLFWwindow;
 * - создание и хранение OpenGL-контекста;
 * - обработку событий GLFW;
 * - обмен front/back buffers;
 * - хранение текущего размера framebuffer;
 * - вычисление соотношения сторон;
 * - предоставление native GLFWwindow для прямой работы с GLFW API.
 *
 * Объект Window владеет указателем GLFWwindow,
 * поэтому копирование класса запрещено.
 */
class Window {
public:
    /**
     * @brief Создаёт окно и инициализирует OpenGL-контекст.
     *
     * В конструкторе создаётся GLFWwindow указанного размера
     * и выполняется необходимая инициализация OpenGL.
     *
     * @param width Начальная ширина окна.
     * @param height Начальная высота окна.
     * @param title Заголовок окна.
     */
    Window(int width, int height, const char* title);

    /**
     * @brief Уничтожает окно и освобождает связанные ресурсы.
     *
     * Деструктор отвечает за уничтожение GLFWwindow
     * и завершение работы GLFW.
     */
    ~Window();

    /**
     * @brief Запрещает копирующее создание Window.
     *
     * Один объект Window связан с конкретным GLFWwindow
     * и соответствующим OpenGL-контекстом,
     * поэтому копирование объекта не допускается.
     */
    Window(const Window&) = delete;

    /**
     * @brief Запрещает копирующее присваивание Window.
     *
     * Это предотвращает появление нескольких объектов,
     * владеющих одним и тем же GLFWwindow.
     */
    Window& operator=(const Window&) = delete;

    /**
     * @brief Проверяет, должен ли Window завершить работу.
     *
     * Метод получает состояние закрытия
     * принадлежащего объекту GLFWwindow.
     *
     * @return true, если окно получило запрос на закрытие;
     * иначе false.
     */
    bool ShouldClose() const;

    /**
     * @brief Обрабатывает события GLFW.
     *
     * Метод передаёт управление системе событий GLFW,
     * которая обрабатывает ввод пользователя,
     * изменение размера окна и другие оконные события.
     */
    void PollEvents();

    /**
     * @brief Выполняет обмен front и back buffers.
     *
     * Метод вызывается после завершения формирования кадра,
     * чтобы отобразить содержимое back buffer пользователю.
     */
    void SwapBuffers();

    /**
     * @brief Возвращает текущую ширину framebuffer.
     *
     * @return Ширина framebuffer в пикселях.
     */
    int GetWidth() const;

    /**
     * @brief Возвращает текущую высоту framebuffer.
     *
     * @return Высота framebuffer в пикселях.
     */
    int GetHeight() const;

    /**
     * @brief Возвращает текущее соотношение сторон framebuffer.
     *
     * Соотношение вычисляется как:
     *
     * width / height.
     *
     * @return Текущее соотношение ширины к высоте.
     */
    float GetAspectRatio() const;

    /**
     * @brief Предоставляет прямой доступ к native GLFWwindow.
     *
     * Метод используется в случаях,
     * когда другой части приложения необходимо обратиться
     * непосредственно к GLFW API.
     *
     * Например, Application использует GLFWwindow
     * для чтения состояния клавиатуры.
     *
     * @return Указатель на принадлежащий объекту GLFWwindow.
     */
    GLFWwindow* GetNativeHandle();

    /**
     * @brief Обрабатывает изменение размера framebuffer.
     *
     * Метод предназначен для обновления OpenGL viewport
     * при изменении размеров framebuffer.
     *
     * @param window GLFWwindow, для которого произошло изменение.
     * @param width Новая ширина framebuffer.
     * @param height Новая высота framebuffer.
     */
    void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    /**
     * @brief Обрабатывает пользовательский ввод для GLFWwindow.
     *
     * Метод предназначен для обработки состояния клавиатуры
     * непосредственно через GLFW API.
     *
     * @param window GLFWwindow, для которого выполняется обработка ввода.
     */
    void processInput(GLFWwindow* window);

private:
    /**
     * @brief Native-указатель на окно GLFW.
     *
     * Window владеет этим ресурсом
     * до выполнения своего деструктора.
     */
    GLFWwindow* handle_ = nullptr;

    /**
     * @brief Текущая ширина framebuffer.
     */
    int width_ = 0;

    /**
     * @brief Текущая высота framebuffer.
     */
    int height_ = 0;
};