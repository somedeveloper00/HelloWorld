cmake_minimum_required (VERSION 4.0)
project(GameEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(vendors/glad)
add_subdirectory(vendors/glm)
add_subdirectory(vendors/glfw)
add_subdirectory(engine)
add_subdirectory(windows)