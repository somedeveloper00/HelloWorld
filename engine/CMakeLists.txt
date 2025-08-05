add_library(engine INTERFACE)

target_include_directories(engine INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# link to opengl and glad
target_link_libraries(engine INTERFACE glad ${OPENGL_gl_LIBRARY})

# for IDE intellisense
target_sources(engine INTERFACE)