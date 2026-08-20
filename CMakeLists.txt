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

# На macOS OpenGL помечен Apple как deprecated.
# Мы сознательно используем OpenGL 3.3 в рамках движка.
if(APPLE)
    add_compile_definitions(GL_SILENCE_DEPRECATION)
endif()


# GLFW

find_package(
        glfw3
        REQUIRED
)


# Qt

find_package(
        Qt6
        REQUIRED
        COMPONENTS
        Widgets
        OpenGLWidgets
)


# ENGINE

add_subdirectory(
        src/Engine
)


# EDITOR

add_subdirectory(
        src/Editor
)


# STANDALONE ENGINE APPLICATION

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