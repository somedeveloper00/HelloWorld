add_library(vtune 
    src/ittnotify/ittnotify_static.c 
    src/ittnotify/jitprofiling.c 
)
target_include_directories(vtune PUBLIC include)
