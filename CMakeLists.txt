cmake_minimum_required(VERSION 3.20)

project(
        MultithreadedMiniEngine
        LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Нужен Qt MOC для классов Qt, использующих Q_OBJECT.
set(CMAKE_AUTOMOC ON)


# macOS

if(NOT APPLE)
    message(
            FATAL_ERROR
            "MultithreadedMiniEngine currently supports macOS only."
    )
endif()

# На macOS OpenGL помечен Apple как deprecated.
# OpenGL 3.3 используется в проекте намеренно.
add_compile_definitions(
        GL_SILENCE_DEPRECATION
)


# Dependencies

include(FetchContent)


# Homebrew

find_program(
        HOMEBREW_EXECUTABLE
        brew
)

if(NOT HOMEBREW_EXECUTABLE)
    message(
            FATAL_ERROR
            "Homebrew was not found. Install Homebrew and configure the project again."
    )
endif()


# GLFW

# Сначала пытаемся использовать уже установленный GLFW.
find_package(
        glfw3
        QUIET
)

# Если GLFW отсутствует, CMake автоматически
# скачивает и собирает его через FetchContent.
if(NOT TARGET glfw)
    message(
            STATUS
            "GLFW not found. Downloading GLFW..."
    )

    set(
            GLFW_BUILD_DOCS
            OFF
            CACHE BOOL ""
            FORCE
    )

    set(
            GLFW_BUILD_TESTS
            OFF
            CACHE BOOL ""
            FORCE
    )

    set(
            GLFW_BUILD_EXAMPLES
            OFF
            CACHE BOOL ""
            FORCE
    )

    FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
            GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(
            glfw
    )
endif()


# Qt6

# Сначала пытаемся найти Qt6 самостоятельно.
find_package(
        Qt6
        QUIET
        COMPONENTS
        Widgets
        OpenGLWidgets
)

# Если Qt6 отсутствует, устанавливаем его через Homebrew.
if(NOT Qt6_FOUND)
    message(
            STATUS
            "Qt6 not found. Installing Qt through Homebrew..."
    )

    execute_process(
            COMMAND
            ${HOMEBREW_EXECUTABLE}
            install
            qt

            RESULT_VARIABLE
            QT_INSTALL_RESULT
    )

    if(NOT QT_INSTALL_RESULT EQUAL 0)
        message(
                FATAL_ERROR
                "Failed to install Qt through Homebrew."
        )
    endif()

    # Получаем фактический путь к Qt.
    execute_process(
            COMMAND
            ${HOMEBREW_EXECUTABLE}
            --prefix
            qt

            OUTPUT_VARIABLE
            QT_HOMEBREW_PREFIX

            OUTPUT_STRIP_TRAILING_WHITESPACE

            RESULT_VARIABLE
            QT_PREFIX_RESULT
    )

    if(NOT QT_PREFIX_RESULT EQUAL 0)
        message(
                FATAL_ERROR
                "Failed to determine the Homebrew Qt installation path."
        )
    endif()

    # Добавляем установленный Qt в пути поиска CMake.
    list(
            PREPEND
            CMAKE_PREFIX_PATH
            "${QT_HOMEBREW_PREFIX}"
    )

    # Повторно ищем Qt после установки.
    find_package(
            Qt6
            REQUIRED
            COMPONENTS
            Widgets
            OpenGLWidgets
    )
endif()


# Engine

add_subdirectory(
        src/Engine
)


# Editor

add_subdirectory(
        src/Editor
)


# Standalone Engine Application

add_executable(
        mini_engine

        src/main.cpp
)

target_include_directories(
        mini_engine

        PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(
        mini_engine

        PRIVATE
        mini_engine_core
)


# Documentation

option(
        MINI_ENGINE_BUILD_DOCS
        "Enable Mini Engine documentation target"
        ON
)

if(MINI_ENGINE_BUILD_DOCS)

    # Сначала пытаемся найти Doxygen.
    find_package(
            Doxygen
            QUIET
    )

    # Если Doxygen отсутствует,
    # устанавливаем его автоматически через Homebrew.
    if(NOT Doxygen_FOUND)
        message(
                STATUS
                "Doxygen not found. Installing Doxygen through Homebrew..."
        )

        execute_process(
                COMMAND
                ${HOMEBREW_EXECUTABLE}
                install
                doxygen

                RESULT_VARIABLE
                DOXYGEN_INSTALL_RESULT
        )

        if(NOT DOXYGEN_INSTALL_RESULT EQUAL 0)
            message(
                    FATAL_ERROR
                    "Failed to install Doxygen through Homebrew."
            )
        endif()

        # Повторно ищем Doxygen после установки.
        find_package(
                Doxygen
                REQUIRED
        )
    endif()


    # Doxygen configuration

    set(
            DOXYGEN_PROJECT_NAME
            "Multithreaded Mini Engine"
    )

    set(
            DOXYGEN_PROJECT_BRIEF
            "Multithreaded interactive 3D visualization engine"
    )

    set(
            DOXYGEN_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/docs"
    )

    set(
            DOXYGEN_GENERATE_HTML
            YES
    )

    set(
            DOXYGEN_GENERATE_LATEX
            NO
    )

    set(
            DOXYGEN_RECURSIVE
            YES
    )

    set(
            DOXYGEN_EXTRACT_ALL
            YES
    )

    set(
            DOXYGEN_EXTRACT_PRIVATE
            YES
    )

    set(
            DOXYGEN_EXTRACT_STATIC
            YES
    )

    set(
            DOXYGEN_SOURCE_BROWSER
            YES
    )

    set(
            DOXYGEN_INLINE_SOURCES
            NO
    )

    set(
            DOXYGEN_CLASS_DIAGRAMS
            YES
    )

    set(
            DOXYGEN_GENERATE_TREEVIEW
            YES
    )

    set(
            DOXYGEN_FULL_SIDEBAR
            YES
    )

    set(
            DOXYGEN_DISABLE_INDEX
            NO
    )


    # Генерация документации.

    doxygen_add_docs(
            generate_docs

            "${PROJECT_SOURCE_DIR}/src"

            COMMENT
            "Generating Mini Engine documentation"
    )


    # Главная страница документации.

    set(
            MINI_ENGINE_DOCS_INDEX
            "${DOXYGEN_OUTPUT_DIRECTORY}/html/index.html"
    )


    # После генерации документация автоматически
    # открывается в браузере macOS.

    add_custom_target(
            docs

            COMMAND
            open
            "${MINI_ENGINE_DOCS_INDEX}"

            DEPENDS
            generate_docs

            COMMENT
            "Opening Mini Engine documentation"
    )

    message(
            STATUS
            "Doxygen documentation target enabled: docs"
    )

endif()