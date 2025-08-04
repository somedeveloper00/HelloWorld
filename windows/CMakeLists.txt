add_executable(windows 
    src/entry.cpp
)
target_include_directories(windows PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(windows PRIVATE engine)