cmake_minimum_required(VERSION 3.20)

project(
        MultithreadedMiniEngine
        VERSION 0.1.0
        LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(
        MINI_ENGINE_BUILD_EDITOR
        "Build the Qt editor"
        ON
)

option(
        MINI_ENGINE_BUILD_RUNTIME
        "Build the standalone GLFW runtime"
        ON
)

find_package(glfw3 REQUIRED)

if(MINI_ENGINE_BUILD_EDITOR)
    find_package(
            Qt6
            REQUIRED
            COMPONENTS
            Widgets
    )
endif()

add_subdirectory(src/Engine)

if(MINI_ENGINE_BUILD_EDITOR)
    add_subdirectory(src/Editor)
endif()

if(MINI_ENGINE_BUILD_RUNTIME)
    add_executable(
            mini_engine_runtime

            src/main.cpp
    )

    target_link_libraries(
            mini_engine_runtime
            PRIVATE
            mini_engine_core
    )

    target_include_directories(
            mini_engine_runtime
            PRIVATE
            ${PROJECT_SOURCE_DIR}/src
    )
endif()