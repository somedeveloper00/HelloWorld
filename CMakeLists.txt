cmake_minimum_required (VERSION 4.0)

project(GameEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   

find_package(OpenGL REQUIRED)

add_subdirectory(glad)
add_subdirectory(engine)
add_subdirectory(windows)