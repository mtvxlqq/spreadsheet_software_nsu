add_library(ss_app STATIC
    src/keymap.cpp
)
target_include_directories(ss_app PUBLIC include)
target_link_libraries(ss_app PUBLIC ss_core)
ss_set_warnings(ss_app)
