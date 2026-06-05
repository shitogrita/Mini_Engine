cmake_minimum_required(VERSION 3.20)

project(MultithreadedMiniEngine LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 REQUIRED)

add_executable(mini_engine
        src/main.cpp
        src/Engine/Core/Application.cpp
        src/Engine/Core/Window.cpp

        src/Engine/Math/affine_transformation.cpp
        src/Engine/Math/projection.cpp

        src/Engine/Platform/OpenGL/Glad/glad.c
)

target_include_directories(mini_engine PRIVATE
        src
        src/Engine/Platform/OpenGL/Glad
)

target_link_libraries(mini_engine PRIVATE
        glfw
)

if(APPLE)
    target_link_libraries(mini_engine PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()