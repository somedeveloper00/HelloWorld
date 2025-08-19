add_library(engine INTERFACE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_include_directories(engine INTERFACE include)
target_link_libraries(engine INTERFACE glad glm::glm glfw)

# for IDE support (this generates a translation unit target for the `compile_commands.json` file including all the above libraries)
target_sources(engine INTERFACE src/dummy.cpp)
