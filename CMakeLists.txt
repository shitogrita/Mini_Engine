cmake_minimum_required(VERSION 3.20)

project(MultithreadedMiniEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mini_engine
        src/main.cpp
        src/Engine/Core/Application.cpp
        src/Engine/Platform/OpenGL/Glad/glad.c
)

target_include_directories(mini_engine PRIVATE
        src
)